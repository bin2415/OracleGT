# Distributions of different ground truths that measure popular disassemblers.

We present the distributions of F1 score [1] of popular disassemblers on instruction recovery, function detection, and jump table reconstruction on the x86/x64 binaries presented in [2].


## Instructions

<p align="center">
<img src="./figs/distribution_gts_instr.png" alt="distribution_gts_instr" width="600"/>
</p>

Figure 1 shows the distributions of F1 score of dissassemblers on instruction recovery using different ground truths. We use three ground truths: leveraging debug info based on [3], use objdump with symbol information and OracleGT. It shows that different ground truths impact the distribution of F1 score of instruction recovery among popular disassemblers.


## Functions

<p align="center">
<img src="./figs/distribution_gts_func.png" alt="distribution_gts_func" width="600"/>
</p>

Figure 2 shows the distributions of F1 score of disassemblers on function detection using different ground truths. We use two ground truths: leverage symbol information and OracleGT.

## Jump Tables

<p align="center">
<img src="./figs/distribution_gts_jmptbl.png" alt="distribution_gts_jmptbl" width="600"/>
</p>

Figure 3 shows the distributions of F1 score of disassemblers on jump table reconstruction using different ground truths. We use two ground truths: IDA Pro with symbol information and OracleGT. The distributions vary especially at optimization level O0.


# Reference

- [1] What is the F1-score: https://www.educative.io/edpresso/what-is-the-f1-score#:~:text=The%20F1%2Dscore%20combines%20the,classifier%20B%20has%20higher%20precision.
- [2] Chengbin Pang, Ruotong Yu, Yaohui Chen, Eric Koski- nen, Georgios Portokalidis, Bing Mao, and Jun Xu. Sok: All you ever wanted to know about x86/x64 binary dis- assembly but were afraid to ask. In 2021 IEEE Sym- posium on Security and Privacy (SP), pages 833–851. IEEE, 2021.
- [3] Dennis Andriesse, Xi Chen, Victor Van Der Veen, Asia Slowinska, and Herbert Bos. An in-depth analysis of disassembly on full-scale x86/x64 binaries. In 25th {USENIX} Security Symposium ({USENIX} Security 16), pages 583–600, 2016.

