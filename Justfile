default:
    @just -l

clean:
    qmk clean

flash-lily58:
    @qmk flash -kb bhekb/lily58_2040/rp  -km macvim -bl uf2-split-left
