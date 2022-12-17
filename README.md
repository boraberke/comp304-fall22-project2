# COMP304 - Fall 2022 - Project 2

This is the second project of the COMP304 course (Fall '22) in Koç University.

## Contributors
Bora Berke Şahin\
Büşra Işık

## Compiling

Use the following to compile and run the parts individually. Log file under the name `events.log` will be created.

### Part 1
```bash
gcc project2.c -o part1
./part1
```

### Part 2
```bash
gcc project_2_partII.c -o part2
./part2
```

### Part 3
```bash
gcc project_2_partIII.c -o part3
./part3
```




An example printout of the queues at time 10:
```
At 23 sec painting: 40, 44
packaging is empty
assembly is empty
qa is empty
At 23 sec delivery: 46
```
An example `events.log` file:
```
Task ID    Gift ID      Gift Type   Task Type  Request Time     Task Arrival    TT     Responsible
1             1             3           A           0                0           3          B
2             2             2           P           1                1           3          A
3             3             1           C           2                2           2          B
4             1             3           C           0                3           2          A
6             2             2           C           1                4           1          B
7             3             1           D           2                4           2          S
8             5             1           C           5                5           1          A
9             1             3           D           0                5           2          S
5             4             3           A           4                4           3          B
```

