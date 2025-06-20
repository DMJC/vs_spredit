# Vegastrike Sprite Editor
Written in C++ using gtkmm-3.0.
Compile with make.

## Asset configuration
The editor looks for a file called `vs_anim.cfg` in the working directory. This
file should contain the path to the root of your sprite assets. You may supply
an absolute path or a path relative to the program's start directory. The
repository includes `vs_anim.cfg` with a sample relative path:

```
assets
```

Edit `vs_anim.cfg` to point to your own sprite directory before running the
application.
