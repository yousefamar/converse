converse
========

A command-line IM chat-like front end to absolutely anything.

`usage: converse input_file output_file`

Converse was written to play nice with apps that adhere to the Unix philosophy like [ii](http://tools.suckless.org/ii/) or [ratox](http://ratox.2f30.org/). `input_file` can for example be a FIFO into which you funnel any text you like. These are pushed to converse's message log. Any text you input in converse's bottom text input area is appened to `output_file`.

With the magic of scripting and converse, you could for example log and reply to tweets, or chat with Cleverbot, or *converse* with anything that will let you, through a single interface and without any screen/tmux hackery!


FAQ
---

**How do I compile?**

Converse is a tiny, single-file program written in ANSI C. It has two dependencies:

  - *ncurses*; for a text-based UI in your terminal
  - *pthread*; for multithreading

Both are already installed most places. To compile, all you need to do is run `make` in the same directory.

**How do I customize the UI?**

To customize anything, just modify the code directly and recompile. It's super simple and thoroughly documented for a reason!

**Why not use stdin and stdout for something like `input-file | converse | output-file`?**

It would be great if that were possible but it's not, since `stdout` is used to display the UI in ncurses and would look like a bunch of escape codes. Likewise `stdin` is used to read user keyboard input. Named pipes to the rescue!

**Can I have multiple inputs and outputs?**

That's entirely possible and should be done outside of converse. Use I/O redirection, named pipes, and commands like `tee`. You could even write your own middleman scripts that intelligently modify and reroute I/O. The freedom of choice is yours!


Coming Soon
-----------

  - Scroll support
  - External editor input support
  - Option to compile with window alert on message support

<!-- vim: set wrap linebreak spell : -->
