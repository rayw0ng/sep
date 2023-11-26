# sep
simple eye protect timer
# feature
- CLI interface
- notify thought Libnotify
- run as daemon
# usage
```sh
# run as daemon
sep start work_minutes break_minutes
# stop daemon
sep stop
# skip this break
sep skip
sep status
```
# integrate with i3status and sway(or i3)
put `sep` to `$HOME/bin/`, copy `myi3status` to `$HOME/bin/` and replace `i3status` in sway's config file with `$HOME/bin/myi3status`.