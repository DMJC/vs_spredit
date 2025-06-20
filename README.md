# Vegastrike Sprite Editor

This project is a small sprite editor written in C++.

## Dependencies

- `gtkmm-3.0`
- `cairomm-1.0`

Both libraries must be available to `pkg-config`.

## Building

Simply run `make build` to build the editor:

```sh
make build
```

## Configuration

The application reads a file named `vs_anim.cfg` from the same directory at
start-up. This file should contain a single line with the path to the root of
your Vegastrike assets, e.g.

```
/home/user/Vegastrike/assets
```

Edit `vs_anim.cfg` to point to your own asset directory before launching the
program.
