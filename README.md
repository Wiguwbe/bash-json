# Bash JSON builtins

This project adds builtins to Bash to handle more complex data structures,
such as those present in JSON (dicts and lists, with more depth).

The builtins are prepended with the 'j' letter (as you can see).

This works by keeping an internal list of objects and returning to the shell
the index of the value on the list, from this point forward called a `handler`.

> The handlers are in the format of `j:<number>`

These functions may return either a handler or the actual string value (if it's a simple value).

Some functions may return an array of strings (`jkeys` for example), instead of a handler, or even not returning anything.

> The functions that take a handler as first argument can also, instead, take a handler from STDIN,
> this only happens if the argument is not provided AND the input is not a tty.
>
> The same happens in `jcmp`, where the first value can also be a JSON literal from STDIN.

> For literal values taken from CLI arguments (below denoted as `key`, `index` or `JSON`),
> a shell format can be passed: a single string literal doesn't need to be quoted.
>
> JSON handlers (not strings) can still be passed.

## List of builtins

Top level functions:
- `jload [<file>]`: loads JSON from file or stdin, returns a handler;
- `jprint <handler>`: prints the value of the handler, in JSON format;
- `junload`: unload a handler from the values **(not implemented)**;
- `jhandler <handler>`: test if the value is a handler.

> Although there is a `jload` command, most of below commands should also be able
> receive JSON as input (mostly to be able to parse ints and strings and stuff).
> See note above.

To handle collections (both dict and list):
- `jnew <-d|-l>`: creates either a dict or list (specified from option), maybe with an initial value?
- `jtype <handler>`: returns 'list', 'dict' or 'unknown' (and returns 1);
- `jget <handler> <key|index>`: get item from collection, returns either a handler or final value;
- `jset <handler> <key|index> <JSON|handler>`: set an item in collection, if is list and index is `-1` it appends. Dicts don't allow appending;
- `jdel <handler> <key|index>`: delete an item from collection (identified by its key/index);
- `jlen <handler>`: get the lenght of the dict/list;
- `jcmp <JSON|handler> <JSON|handler>`: compares two JSON objects;
- `jkeys <handler>`: get an array of the keys of the dict, function not implemented for lists. Returns a string array;
- `jvalues <handler>`: get the array of values, returns a string array;
- `jhaskey <handler> <key>`: returns wether the dict has the key, not implemented for lists;
- `jhasval <handler> <JSON|handler>`: returns wether the collection has the value;

> There's not a lot of robustness implemented, beware of dragons.

## Building

It doesn't have dependencies, it just needs the bash headers installed,
the default location is `/usr/include/bash` and can be changed in the
Makefile.

Simply running `make` should do it.

## Running

First thing is to enable the builtins, there's a file `enable.sh` to ease that,
simple change the `SO_LOCATION` variable, if you need it.

You can also change the default list of builtins to load.

This file should be `source`d from the bash shell, you can source it from your `~/.bashrc`.

## Why?

I always wanted to handle bigger and more complex data structures from bash itself. I tried
some other shells, like `xonsh`, but the learning curve felt to big. It also felt like there
was a separation between "python" and "shell" code ... maybe I got it wrong.

So, because of that and me liking a challenge, I did this. _smiley emoji_

## Technical Details

Because bash, with pipes and stuff, forks itself, even to run the builtins, it gets quite difficult
to create JSON objects (as C structs in memory) on a child process (like `cat file.json | jload`,
`jload` gets executed in a child process), so this needed a new approach to memory.

Because I didn't want to create global (hidden) files or anything like it (I wanted to keep the session's
objects inside that session only), the memory model was created around `POSIX shared memory` (`shm_overview(7)`).

With shared memory, a few questions arise:
- it has a file descriptor interface, it can me `mmap`ed to memory, but the addresses change every time (or at least is assumed);
- there aren't any JSON libraries (that I could find) that use shared memory interfaces;
- there aren't any memory allocators that work on shared memory (I didn't look too hard).

So the addresses need to be relative. And a memory allocator is needed.

This is implemented in the files `shmalloc.c` and `shmalloc.h`. It uses relative addresses and, when needed, can provide
a (actual) pointer to the target region.

Then a basic JSON library was created to use the shared memory library, with relative addresses. This is implemented
in `json.c` and `json.h`, with `json-parser.h` and `json-parser.c` to parse the JSON (see Cheers below).

> I may release these as standalone libraries, in the meantime just copy the files to your project.

To create the shared memory, and later unlink it, ELF constructors and destructors were used. The shared memory
is named after the PID of the process that loads the `.so`. The destructor gets called every time the `.so` is
release, which also happens everytime that a fork exits. To fight that, the destructor only unlinks the
shared memory if the PID of the process matches the one that created it.

## Cheers

Cheers to [Martin Mitás](https://github.com/mity) for `json-parser.c` and `json-parser.h` (formely `json.c` and `json.h`) from [centijson](https://github.com/mity/centijson).
