script_path=$0
base=`dirname $script_path`
disassembler=${base}/../disassemblers/bap/bapBB.py

binutils="/work/arm32_sync/testsuite/exeutables/utils/binutils"

bash $base/run_disassembler.sh -d $binutils/gcc_O0_strip -s $disassembler -p "BlockBap" &
bash $base/run_disassembler.sh -d $binutils/gcc_O2_strip -s $disassembler -p "BlockBap"
bash $base/run_disassembler.sh -d $binutils/gcc_O3_strip -s $disassembler -p "BlockBap" &
bash $base/run_disassembler.sh -d $binutils/gcc_Os_strip -s $disassembler -p "BlockBap"
bash $base/run_disassembler.sh -d $binutils/gcc_Of_strip -s $disassembler -p "BlockBap" &

bash $base/run_disassembler.sh -d $binutils/clang_O0_strip -s $disassembler -p "BlockBap"
bash $base/run_disassembler.sh -d $binutils/clang_O2_strip -s $disassembler -p "BlockBap" &
bash $base/run_disassembler.sh -d $binutils/clang_O3_strip -s $disassembler -p "BlockBap" 
bash $base/run_disassembler.sh -d $binutils/clang_Os_strip -s $disassembler -p "BlockBap" &
bash $base/run_disassembler.sh -d $binutils/clang_Of_strip -s $disassembler -p "BlockBap"

wait
cpu2006="/work/arm32_sync/testsuite/exeutables/utils/cpu2006"

bash $base/run_disassembler.sh -d $cpu2006/gcc_O0_strip -s $disassembler -p "BlockBap" &
bash $base/run_disassembler.sh -d $cpu2006/gcc_O2_strip -s $disassembler -p "BlockBap"
bash $base/run_disassembler.sh -d $cpu2006/gcc_O3_strip -s $disassembler -p "BlockBap" &
bash $base/run_disassembler.sh -d $cpu2006/gcc_Os_strip -s $disassembler -p "BlockBap"
bash $base/run_disassembler.sh -d $cpu2006/gcc_Of_strip -s $disassembler -p "BlockBap" &

bash $base/run_disassembler.sh -d $cpu2006/clang_O0_strip -s $disassembler -p "BlockBap"
bash $base/run_disassembler.sh -d $cpu2006/clang_O2_strip -s $disassembler -p "BlockBap" &
bash $base/run_disassembler.sh -d $cpu2006/clang_O3_strip -s $disassembler -p "BlockBap" 
bash $base/run_disassembler.sh -d $cpu2006/clang_Os_strip -s $disassembler -p "BlockBap" &
bash $base/run_disassembler.sh -d $cpu2006/clang_Of_strip -s $disassembler -p "BlockBap"

wait
