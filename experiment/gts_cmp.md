# Comparing Different Ground Truth Approaches

Below, we compare the false negatives and false positives produced by different ground truth approaches. In this part, we considered our OracleGT as the ground truth. In particular, we manually validated the results reported by SoK (reference [25] in our reference).

## Instructions

| **Ground Truth** | **IDA Pro with<br> Symbol info** | **Objdump with<br> Symbol info** | **Usenix’16 (reference [4]<br> in our paper)** | **SoK [25]** | **OracleGT** |
| ----- | :-----: | :-----: | :----: | :----: | :-----: |
| # of False Negatives | 379,704 | 2,107 | 5,259,167 | 2,388 | 0 |
| # of False Positives | 2,069 | 136,665 |  4,417 | 0 | 0 |

## Functions


| **Ground Truth** | **Symbol Info** | **SoK [25]** | **OracleGT** |
| ----- | :-----: | :-----: | :----: |
| # of False Negatives | 5 | 5 | 0 |
| # of False Positives | 38,715 |  0 | 0 |

## Jump Tables

| **Ground Truth** | **IDA Pro with<br> Symbol info** | **Usenix’16 (reference [4]<br> in our paper)** | **SoK [25]** | **OracleGT** |
| ----- | :-----: | :-----: | :----: | :----: |
| # of False Negatives | 286 | 43,490 | 515 | 0 |
| # of False Positives | 290 | 13,678 |  0 | 0 |
