# Compiler

## Porting to LLVM/Clang

```

 ===============       ================       ================      ==========      ==============      ==========
||              ||    ||               ||    ||              ||    ||        ||    ||            ||    ||        ||
||    Machine   || => ||    Machine    || => || MachineInstr || => || MCInst || => || MCFragment || => || Object ||
||   Function   ||    ||   BasicBlock  ||    ||              ||    ||        ||    ||            ||    ||        ||
||              ||    ||               ||    ||              ||    ||        ||    ||            ||    ||        ||
 ===============       ================       ================      ==========      ==============      ==========
                                                     MC Componment
```

To identify the boundary of every basic block inside object file, the tool uses the unique identifier(MFID_MBBID) to represent every `MachineBasicBlock` and pass
this identify to `MachineInstr` and `MCInst`. The generation of identifier is shown as below:

```c++
std::string GetMBBInst(const MachineInstr *MI)
{
  const MachineBasicBlock *MBB = MI->getParent();
  unsigned MBBID = MBB->getNumber();
  unsigned MFID = MBB->getParent()->getFunctionNumber();
  std::string ID = std::to_string(MFID) + "_" + std::to_string(MBBID);
  return ID;//the parent ID of this MCInst: "MFID_MBBID"
}
```

### Modification of MCAsmInfo

`MCAsmInfo` class is used as a base class for assembly properties and features of specific architecture.

In order to record the information of every basic block, such as size, offset from `fragment` and type, the tool adds identifiers and methods in `MCAsmInfo`.

```c++
class MCAsmInfo {
...
//Essential bookkeeping information for reordering in the future (installation time)
  // (a) MachineBasicBlocks (map)
  //    * MFID_MBBID: <size, offset, # of fixups within MBB, alignments, type, sectionName, contains inline assemble>
  //    - The type field represents when the block is the end of MF or Object where MBB = 0, MF = 1, Obj = 2, and if now block is special mode all type add 1 << 6 such as TBB(thumb basic block) = 64 and TF(thumb function) = 65
  //    - The sectionOrdinal field is for C++ only; it tells current BBL belongs to which section!
  //      MBBSize, MBBoffset, numFixups, alignSize, MBBtype, sectionName, assembleType
  mutable std::map<std::string, std::tuple<unsigned, unsigned, unsigned, unsigned, unsigned, std::string, unsigned>> MachineBasicBlocks;
  //    * MFID: fallThrough-ability
  mutable std::map<std::string, bool> canMBBFallThrough;
  //    * MachineFunctionID: size
  mutable std::map<unsigned, unsigned> MachineFunctionSizes;
  //    - The order of the ID in a binary should be maintained layout because it might be non-sequential.
  mutable std::list<std::string> MBBLayoutOrder;

  // (b) Fixups (list)
  //    * <offset, size, isRela, parentID, SymbolRefFixupName, isNewSection, secName, numJTEntries, JTEntrySz>
  //    - The last two elements are jump table information for FixupsText only,
  //      which allows for updating the jump table entries (relative values) with pic/pie-enabled.
  mutable std::list<std::tuple<unsigned, unsigned, bool, std::string, std::string, bool, std::string, unsigned, unsigned>>
          FixupsText, FixupsRodata, FixupsData, FixupsDataRel, FixupsInitArray; 
  //    - FixupsEhframe, FixupsExceptTable; (Not needed any more as a randomizer directly handles them later on)
  //    - Keep track of the latest ID when parent ID is unavailable
  mutable std::string latestParentID;
  
  // (c) Others
  //     The following method helps full-assembly file (*.s) identify functions and basic blocks
  //     that inherently lacks their boundaries because neither MF nor MBB has been constructed.
  mutable bool isAssemFile = false;
  mutable bool hasInlineAssembly = false;
  mutable std::string prevOpcode;
  mutable unsigned assemFuncNo = 0xffffffff;
  mutable unsigned assemBBLNo = 0;
  mutable unsigned specialCntPriorToFunc = 0;


  void updateOffset(std::string id,unsigned offset) const  {
    std::get<1>(MachineBasicBlocks[id]) = offset;
  }
  void updateInlineAssembleType(std::string id, unsigned type) const{
    if (MachineBasicBlocks.count(id) == 0) {
      MachineBasicBlocks[id] = std::make_tuple(0, 0, 0, 0, 0, "", 0);
    }
    std::get<6>(MachineBasicBlocks[id]) = type;
  }
    // Update emittedBytes from either DataFragment, RelaxableFragment or AlignFragment
  bool updateByteCounter(std::string id, unsigned emittedBytes, unsigned numFixups, \
                         bool isAlign, bool isInline, bool isSpecialMode = false) const {
    // std::string id = std::to_string(fnid) + "_" + std::to_string(bbid);
    // Create the tuple for the MBB
    bool res = false;
    if (MachineBasicBlocks.count(id) == 0) {
      MachineBasicBlocks[id] = std::make_tuple(0, 0, 0, 0, 0, "", 0);
      res = true;
    }

    // Otherwise update MBB tuples
    std::get<0>(MachineBasicBlocks[id]) += emittedBytes; // Acutal size in MBB
    std::get<2>(MachineBasicBlocks[id]) += numFixups;    // Number of Fixups in MBB
    if (isAlign)
      std::get<3>(MachineBasicBlocks[id]) += emittedBytes;  // Count NOPs in MBB

    // If inlined, add the bytes in the next MBB instead of current one
    if (isInline)
      std::get<0>(MachineBasicBlocks[latestParentID]) -= emittedBytes;

    //If it is currently a special mode, modify the type identifier
    if(isSpecialMode)
      std::get<4>(MachineBasicBlocks[id]) |= 1 << 6;
    return res;
  }
...
}
```

### Modification of MCObjectFileInfo

`MCObjectFileInfo` class describes the information of object file, the tool declares a variable to store the jump table information inside the class.

```c++
class MCObjectFileInfo {
    ...
	MCSection *RandSection; //Special section .rand to store additional information
	..
    //Contains all JumpTables whose entries consist of the target MFs and MBBs
  	//<MachineFunctionIdx_JumpTableIdx> - <(EntryKind, EntrySize, Entries[MFID_MBBID])>
 	mutable std::map<std::string, std::tuple<unsigned, unsigned, std::list<std::string>>> JumpTableTargets;
    std::map<std::string, std::tuple<unsigned, unsigned, std::list<std::string>>> \
        getJumpTableTargets() const { return JumpTableTargets; }

    void updateJumpTableTargets(std::string Key, unsigned EntryKind, unsigned EntrySize, \
                                std::list<std::string> JTEntries) const {
        JumpTableTargets[Key] = std::make_tuple(EntryKind, EntrySize, JTEntries);
    }
}
```

To record these information, the tool adds `MachineFunction::RecordMachineJumpTableInfo` function.

```c++
// As optimization goes, MJTI might keep being updated from the followings
//        a) MachineFunctionPass::SelectionDAGISel::XXXDAGToDAGISel::runOnMachineFunction()
//        b) MachineFunctionPass::BranchFolderPass::runOnMachineFunction() 
void MachineFunction::RecordMachineJumpTableInfo(MachineJumpTableInfo *MJTI) {
  if(jump_table in MJTI)
  {
  	updateJumpTableTargets(jump_table.id,jump_table.entryKind,jump_table.entrySize,jump_table.JTEntries)
  }
}
class XXXDAGToDAGISel::AArch64DAGToDAGISel : public SelectionDAGISel {
	bool runOnMachineFunction(MachineFunction &MF) override {
	...
    MachineJumpTableInfo *MJTI = MF.getJumpTableInfo();//Get the jumptable info of function
    if (MJTI)
      MF.RecordMachineJumpTableInfo(MJTI); 
  	...
   }
}
```

### Modification of MCInst

In order to pass information to `MCInst`, the tool adds corresponding identifiers and methods inside it.

```c++
class MCInst {
...
    mutable bool special_mode = false; //Identifies whether the current instruction is a special mode, such as thumb
    mutable unsigned JumpTableSize = 0; //Record the size of the current jump table, if it is 0, it means that the current instruction is not a special jump table instruction

    mutable unsigned byteCtr = 0;//current instruction length
    mutable unsigned fixupCtr = 0; //The size of the current fixup
    std::string ParentID;//Identifies the BB of the current instruction,

    std::string TableSymName;//Jump table symbol information related to the current instruction
... 
    void setSpecialMode(bool Mode) const {special_mode = Mode; }//Set the current command special mode information
  	bool getSpecialMode() const {return special_mode; } // Get current command special mode information
    
    void setByteCtr(unsigned numBytes) const { byteCtr = numBytes; }//Set the current instruction length
    unsigned getByteCtr() const { return byteCtr; }//Get the current instruction byte count
    
    void setFixupCtr(unsigned numFixups) const { fixupCtr = numFixups; }//Set the current instruction fixup size
    unsigned getFixupCtr() const { return fixupCtr; }//Get the current instruction fixup size
    
    void setParent(std::string P) { ParentID = P; }//Set the BB identifier of current instruction
  	const std::string getParent() const { return ParentID; }//Get the BB identifier of current instruction
    
    void setJumpTable(int sz) { JumpTableSize =  sz;}//Set the size of the current jump table
    unsigned getJumpTable() const { return JumpTableSize; }//Get the size of the current jump table
    
    void setTableSymName(std::string P) { TableSymName = P; }//Set the Jump table symbol information related to current instruction
    const std::string getTableSymName() const { return TableSymName; }//Get the Jump table symbol information related to current instruction
...
}
```

### Modification of MCInstBuilder

MCInstBuilder is the helper class to create `MCInst`. The tool adds some methods to set the identifier for every generated `MCInst`.

```c++
class MCInstBuilder {
  MCInst Inst;
...
  //Set the BB identifier of current instruction
  MCInstBuilder &setParent(std::string ID) {
    Inst.setParent(ID);
    return *this;
  }
  //Set the size of the current jump table
  MCInstBuilder &setJumpTable(int sz) {
    Inst.setJumpTable(sz);
    return *this;
  }
  //Set the Jump table symbol information related to current instruction
  MCInstBuilder &setTableSymName(std::string SymName) {
    Inst.setTableSymName(SymName);
    return *this;
  }
...
}
```

### Modification of MCInst

`EmitInstruction` is the function that emits `MachineInstr` to the object, the tool hooks the procedure to mark the instruction boundary.

```c++
void tragetArchitecture::EmitInstruction(const MachineInstr *MI) {
    ...
	switch(MI->getOpcode())
    {
        case 1:
            ...
        	MCInst TmpInst;
            ...
            TmpInst.setParent(GetMBBInst(MI));//Declared in MCInst.h
            ...
        	break;
        case 2:
            ...
            EmitToStreamer(*OutStreamer, MCInstBuilder(ARM::Bcc)
              .addInfo(someinfo).
              .setParent(GetMBBInst(MI)));//Declared in MCInstBuilder.h
    }
    ...
}
```

### Record the Information of Jump Table

```c++
void MCELFStreamer::EmitInstToData(const MCInst &Inst,
                                   const MCSubtargetInfo &STI) {
	std::string ID = Inst.getParent(); //Declared in MCInst.h,(MFID_MBBID)
    ...
    for fixup in fixups
    {
        //This part needs special treatment according to different architectures
        //1.Different jump table prefixes
        //2.Different fixup types require special handling
        if(".LJTI" in fixup.sym or "$JTI" in fixup.sym)
        {
            fixups[i].setIsJumpTableRef(true); //Set the fixup to be associated with a jump table
          	fixups[i].setSymbolRefFixupName(fixup.sym);
        }
    }
    for fixup in addedfixups // handle special instruction such as tbb
    {
            fixups[i].setIsJumpTableRef(true);
          	fixups[i].setSymbolRefFixupName(fixup.sym);
    }
}
```

### Write Ground Truth to Binary

To store the ground truth information, the tool creates a new section `.gt` in the binary

```c++
void ELFObjectWriter::writeSectionData(const MCAssembler &Asm, MCSection &Sec,
                                       const MCAsmLayout &Layout) {
  ...
  if (section name is ".gt") {
    Asm.WriteRandInfo(Layout); // write addtion info into .rand section
  }
  ...
}                                      
void WriteRandInfo(Layout)
{
    ...
	if(section name is ".text") // force on .text section
    {
        for fragment in fragments
        {
            totalOffset = fragment.offset
            for BB in BBs
            {
                BB.updateOffset(totalOffset)//update the offset with BBsize and fragment offset
                totalOffset += BB.size
           		function.size += BB.size
               	if BB is function end
                    BB.updateType(func_type) //if BB is func end, update the BB type
            }
        }
    }
    ...
}
Void Layout(layout)
{
    for section in sections
        for fragment in fragments
        {
            for fixup in fixups
                if(jumptable) // if fixup is related to a jumptable,update the info to fixuplist declared in MCAsmInfo.h
                    updateFixuplist();
            for fixup in addedfixups // handle special fake fixup, to record the jumptable
                if(jumptable)
                    updateFixuplist();
        }
}
```

## Porting to GCC

In order to pass information from `GCC` compiler to `GNU Assembler`, The tool defines some `directives`[1] to mark basic block information, function information, inline information and jump table information

| Label          | Information                           |
| -------------- | ------------------------------------- |
| bbInfo_BB      | mark the basic block begin location   |
| bbInfo_BE      | mark the basic block end location     |
| bbInfo_FUNB    | mark the function start location      |
| bbInfo_FUNE    | mark the function end location        |
| bbInfo_JMPTBL  | mark the jump table information       |
| bbInfo_INLINEB | mark the asm inline start information |
| bbInfo_INLINEE | mark the asm inline end information   |

The assembly code generated by instrumented `GCC` is shown as follows:

```assembly
.LFB5:
        .cfi_startproc
        .bbInfo_FUNB
        .bbInfo_BB 0
        pushq   %rbp
        .cfi_def_cfa_offset 16
        .cfi_offset 6, -16
        movq    %rsp, %rbp
        .cfi_def_cfa_register 6
        leaq    .LC0(%rip), %rdi
        call    puts@PLT
        movl    $-1, %edi
        .bbInfo_BE 0
        call    exit@PLT
        .cfi_endproc
.LFE5:
        .bbInfo_FUNE
....
.L10:
        .bbInfo_JMPTBL 35 4
        .long   .L39-.L10
        .long   .L8-.L10
        .long   .L38-.L10
        .long   .L8-.L10
        .long   .L8-.L10
        .long   .L37-.L10
        .long   .L36-.L10
        .long   .L8-.L10
....
```

In order to output these labels, the tool created `bbinfo2asm.c` and instrumented `final.c` and `cfg.c`.

In `bbinfo2asm`, the tool defines the following functions:

```c
// output the basic block begin label
extern void bbinfo2_asm_block_begin(uint32_t);

// output the basic block end label
extern void bbinfo2_asm_block_end(uint32_t);

// output the jump table information, including table size and entry size
extern void bbinfo2_asm_jumptable(uint32_t table_size, uint32_t entry_size);

// output the function begin label
extern void bbinfo2_asm_func_begin();

// output the function end label
extern void bbinfo2_asm_func_end();

// output the asm inline start label
extern void bbinfo2_asm_inline_start();
extern void bbinfo2_asm_inline_end();

```

```c
//final.c
final_start_function_1()
{
    ...
    bbinfo2_asm_func_begin();//Output the bbInfo_FUNC Label
    ...
}
final_end_function()
{
    ...
    bbinfo2_asm_func_end();//Output the bbInfo_FUNE Label
    ,,,
}

dump_basic_block_mark(inst)
{
    flag = 0;
    for edge in edges
        if(edge_fall_through(edge))
			flag = 1;
 	if inst is the first instruction of BB
        bbinfo2_asm_block_begin(flag);//Output the bbInfo_BB Label
    if inst is the last instruction of BB
        bbinfo2_asm_block_end(flag);//Output the bbInfo_BE Label
}
final_1()
{
    ...
    for inst in insts
        dump_basic_block_mark(inst);
    ...
}
app_enable()
{	
    ...
    if (! app_on)
    {
        ...
    	bbinfo2_asm_inline_start();//Output the bbInfo_INLINEB Label
    	...
    }
    ...
}
app_app_disable()
{	
    ...
    if (app_on)
    {
        ...
    	bbinfo2_asm_inline_end();//Output the bbInfo_INLINEE Label
    	...
    }
    ...
}
//And to get basic block information, comment out flag_debug_asm.
```

```c
//cfg.c
// if the edge is fall through, return true
edge_fall_through(edge e){
	if(e.flag is fallthrough)
        return true;
   	return false;
}
```

# Assembler

## Porting to GNU Assembler(GAS)

> Recommendation: Assembler is not related to compiler optimizations, we could leave the `GAS` as it is until it is not fit in new GCC compilers.

The process of assembling could be deemed as a state machine: when processing `directive`, it defines current state and triger the specific action to handle following sequence bytes. So we could add specific `directives` to pass information from compiler to assembler and represent the specific state inside assembler.

Specifically, in order to migrate to new gas, we could do following modifications:

### Define Handler for Directives

```c
const pseudo_typeS bbInfo_pseudo_table[] = {
    {"bbinfo_jmptbl", jmptable_bbInfo_handler, 0}, // handle jump table
    {"bbinfo_funb", funcb_bbInfo_handler, 0},   // handler start of a function
    {"bbinfo_fune", funce_bbInfo_handler, 0},   // handle end of a function
    {"bbinfo_bb", bb_bbInfo_handler, 0},    // handle start of a basic block(bb)
    {"bbinfo_be", be_bbInfo_handler, 0},    // handler end of a bb
    {"bbinfo_inlineb", inlineb_bbInfo_handler, 0},  // handle start of inline pseudo-bb
    {"bbinfo_inlinee", inlinee_bbInfo_handler, 0},  // handle end of inline pseudo-bb
    {NULL, NULL, 0}
};
```

The modified `GCC` emits corresponding `directives` to pass the boundary of function, basic block and the information of jump tables. At the assembler side, we could reconstruct these information when handling specific directive. In order to represent these informaton, we could use following structures:

```c
// basic block related information
struct basic_block{
  uint32_t ID; // basic block id, every basic block has unique id in an object
  uint8_t type; // basic block type: basic block or function boundary.
    // 0 represent basic block with normal mode ie. arm
    // 1 represents function start with normal mode ie. arm
    // 2 represents object end with normal mode ie. arm
    // 4 represent basic block with special mode ie. thumb
    // 5 represents function start with special mode ie. thumb
    // 6 represents object end with special mode ie. thumb
  uint32_t offset; // offset from the section
  int size; // basic block size, include alignment size
  uint32_t alignment; // basic block alignment size
  uint32_t num_fixs; // number fixups
  unsigned char fall_through; // whether the basic block is fall through
  asection *sec; // which section the basic block belongs to
  struct basic_block *next; // link next basic blosk
  uint8_t is_begin; // if current instruction is the first instruction of this basic block
  uint8_t is_inline; // if current basic block contains inline assemble code or current basic block
  fragS *parent_frag; // this basic block belongs to which frag.
};
```

The tool uses `basic_block` to represent the basic unit that contains continuous instructions. When met `bbinfo_bb`, the tool initializes a new `basic_block`:

- Update the `fall_through` field according to the value obtained by `bbinfo_bb` directive.
- `Fragment` is the basic unit inside assembler, it represents continuous fixed regions. The tool associates `basic_block` with fragment when initializing and update the offset inside current fragment.
- Update `sec` field which represents which section it belongs to.

### Record Instructions

The tool hooks the process of emitting instructions into fragment, and record every instruction in current `basic_block`. In `gas/config` directory, it defines architecture related functions to emit insturctions into fragment. For example, for `AArch64`, `gas/config/tc-aarch64.c::output_inst(struct aarch64_inst *new_inst)` function do that work.

```c
// in gas/config/tc-aarch64.c
static void
output_inst (struct aarch64_inst *new_inst)
{
    ...
    frag_now->last_bb = mbbs_list_tail;
    if (mbbs_list_tail) {
        mbbs_list_tail->size += INSN_SIZE; // update current instuction to current basic block
    }
    ...
}
```

### Store Jump Table Information

The tool leverages `fixup` to record the information of jump table. Specifically, when met `bbinfo_jmptbl` directive, it could obtain the information of jump table(The size of jump table and the size of every jump table entry) and associates the information with last `fixup`.

```c
// handle bbinfo_jmptbl directive
void jmptable_bbInfo_handler(int ignored ATTRIBUTE_UNUSED){
    offsetT table_size, entry_size;
    table_size = get_absolute_expression();
    SKIP_WHITESPACE();

    entry_size = get_absolute_expression();
    if (last_symbol == NULL){
	    as_warn("Sorry, the last symbol is null\n");
	    return;
    }

    // update the jump table related information of the symbol
    S_SET_JMPTBL_SIZE(last_symbol, table_size);
    //as_warn("JMPTBL table size is %d\n", table_size);
    S_SET_JMPTBL_ENTRY_SZ(last_symbol, entry_size);
}
```

# Linker

## Porting to Gold Linker

> Recommendation: Linker is not related to compiler optimizations, we could leave the `gold as` it is until it is not fit in new compilers.

Linker integrates object files(.o) into one executable file and updates informations of final executable file(such as relocations). The tool hooks the process of
Gold to updates the offset of every basic block. Specifically, when link finalizes the integration of object files, we update the offsets.

```c++
// in gold/layout.cc
off_t
Layout::finalize(const Input_objects* input_objects, Symbol_table* symtab,
		 Target* target, const Task* task)
{
    ...
    // Run the relaxation loop to lay out sections.
  do
    {
      off = this->relaxation_loop_body(pass, target, symtab, &load_seg,
				       phdr_seg, segment_headers, file_header,
				       &shndx);
      pass++;
    }
  while (target->may_relax()
	 && target->relax(pass, input_objects, symtab, this, task));

  // the part added
  bool is_big_endian = parameters->target().is_big_endian();
  int binary_format_size = parameters->target().get_size();
  if (is_big_endian && binary_format_size == 64){
    this->update_shuffleInfo_layout<64, true>();
  } else if (!is_big_endian && binary_format_size == 64){
    this->update_shuffleInfo_layout<64, false>();
  } else if (is_big_endian && binary_format_size == 32){
    this->update_shuffleInfo_layout<32, true>();
  } else if (!is_big_endian && binary_format_size == 32){
    this->update_shuffleInfo_layout<32, false>();
  }
  ...
}
```

In `update_shuffleInfo_layout()`, the tool iterates every basic block and update its offsets inside executable file.

Finally, the tool hooks the process of generating sections and add section `.gt` to store ground truth of binary disassembly.

```c++
// in gold/main.cc
 std::string rand(".gt=");
  std::string opt_2 = rand+shuffle_bin_gz;
  // binpang, support the `-r` option
  if (parameters->options().relocatable()){
    opt_2 = rand+shuffle_bin;
  }
  char * const add_section[] = {"objcopy", "--add-section", (char *)opt_2.c_str(), (char *)target.c_str(), (char*)NULL};
  if(fork()){
  int status;
  wait(&status);
  }else{
  //child exec the objcopy to integrate shufflebin into target
  execvp("objcopy", add_section);
  _exit(0);
  }

```

# References

- [1] Assembler Directives: https://eng.libretexts.org/Bookshelves/Electrical_Engineering/Electronics/Implementing_a_One_Address_CPU_in_Logisim_(Kann)/02%3A_Assembly_Language/2.03%3A_Assembler_Directives#:~:text=Assembler%20directives%20are%20directions%20to,not%20translated%20into%20machine%20code.