# Typeinfer C

This is a small demo program to experiment with type inferring and showcase a recursive algorithm for complex type inference

# Quickstart

```sh
$ cc nob.c -o nob
$ ./nob 
$ ./main
expr1: (int(3) + (int(4) * a))
expr2: (int(2)[int32] + (int(3) * a))
a : <unknown>;
-- Inferring types --
expr1: (int(3)[int32] + (int(4)[int32] * a[int32])[int32])[int32]
expr2: (int(2)[int32] + (int(3)[int32] * a[int32])[int32])[int32]
a : int32;
```
