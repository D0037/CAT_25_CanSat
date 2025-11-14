# Code for the RP2040 based board

## Compilation:
1. create `build` directory in `rp2040_main`
2. `cd` into it
3. run `cmake ..`
4. run `make rp2040_main -j$(nproc)`
5. put board into `BOOTSEL` mode, and copy the `.uf2` file onto it: `$ cp rp2040_main.uf2 /run/media/$USER/RPI-RP2/`
6. the board will automatically reset, and run your program