default:
    @just -l

clean:
    qmk clean
    @echo "delete all uf2/hex/bin files"
    @find . -maxdepth 1 -iname "*.uf2f" -iname "*.bin" -iname "*.hex"
    @rm -f *.uf2
    @rm -f *.bin
    @rm -f *.hex

flash-lily58:
    @qmk compile -j 20 -kb bhekb/lily58_2040/rp  -km macvim
    @qmk flash -kb bhekb/lily58_2040/rp  -km macvim -bl uf2-split-left
