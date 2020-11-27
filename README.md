# wol

## Usage
1. write target name and physical address at `$HOME/.config/wol_targets`
    ```
    $ echo "target_name    00:11:22:aa:bb:cc" >> $HOME/.config/wol_targets
    ```
2. execute with
    ```
    $ wol target_name
    sent signal to target_name
    ```
