# Contributing

This project uses the MIT license. Submit only material you have permission to
contribute under MIT and preserve third-party notices. No copyright transfer or
separate CLA is required by this contribution guide.

For bug reports, include the compiler, OS, ONNX Runtime version/provider, tensor
signature, configuration, error text and a minimal reproduction. Model names or
file extensions alone are insufficient. Do not attach private recordings or
models without permission.

Keep the C API usable from a C compiler. Changes to audio behavior need a targeted
regression test and a note about model compatibility. Use explicit dependencies;
do not add opaque binaries. Run the commands in `docs/build.md` and record which
real models were tested separately from synthetic test graphs.
