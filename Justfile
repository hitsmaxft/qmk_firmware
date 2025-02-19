default:
    @just -l

flash-lily58:
    @qmk flash -kb lily58_2040/rp  -km macvim -bl uf2-split-left
