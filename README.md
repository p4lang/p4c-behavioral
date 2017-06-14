p4c-behavioral
========

***This repository is
   deprecated. [p4c-behavioral](https://github.com/p4lang/p4c-behavioral) has
   been replaced by the
   [behavioral-model](https://github.com/p4lang/behavioral-model), aka
   bmv2. Please refer to this
   [README](https://github.com/p4lang/behavioral-model/blob/master/README.md)
   for more information. We have stopped providing maintenance for this
   repository and be advised that it may be removed altogether from Github after
   12/31/2017. If you have concerns about transitioning to bmv2, please send an
   email to p4-dev@lists.p4.org.***

P4 compiler for the behavioral model (BM) target

Pre-requisites: [p4-hlir](https://github.com/p4lang/p4-hlir)

To install:
```
sudo python setup.py install
```

To run:
```
p4c-behavioral <path_to_p4_program> --gen-dir <dir_for_generated_files> [--thrift]
```

For more info:
```
p4c-behavioral --help
```

p4c-bm is an alias for p4c-behavioral
