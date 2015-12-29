converse
========

A command-line IM chat-like front end to absolutely anything.

`usage: converse input_file output_file`

Converse was written to play nice with apps that adhere to the Unix philosophy like [ii](http://tools.suckless.org/ii/) or [ratox](http://ratox.2f30.org/). `input_file` can for example be a FIFO into which you funnel any text you like. These are pushed to converse's message log. Any text you input in converse's bottom text input area is appened to `output_file`.

With the magic of scripting and converse, you could for example log and reply to tweets, or chat with Cleverbot, or *converse* with anything that will let you, through a single interface and without any screen/tmux hackery!


FAQ
---

**How do I compile?**

Converse is a tiny, single-file program written in C. It has two dependencies:

  - *ncurses*; for a text-based UI in your terminal
  - *pthread*; for multithreading

Both are already installed most places. To compile, all you need to do is run `make` in the same directory.

**How do I install?**

You guessed it: `sudo make install` after `make`.

**How do I customize the UI?**

Although converse uses ncurses, there is special support for some ANSI escape codes. These include foreground and background set and reset for the eight main terminal colours (black, red, green, yellow, blue, magenta, cyan, and white) and common formatting (bold, dim, underline, blink, reverse, and hidden) as long as your terminal supports them. Terminal defaults are used if something isn't set. Try something like this to check if things work for you:

    echo -e "default \e[30mblack \e[31mred \e[32mgreen \e[33myellow \e[34mblue \e[35mmagenta \e[36mcyan \e[97mwhite\e[39m \e[40mblack \e[41mred \e[42mgreen \e[43myellow \e[44mblue \e[45mmagenta \e[46mcyan \e[30;107mwhite\e[39;49m \e[1mbold\e[21m \e[2mdim\e[22m \e[4munderlined\e[24m \e[5mblink\e[25m \e[7mreverse\e[27m \e[8mhidden\e[28m \e[34;1;4;7mhybrid\e[0m default\e[36m" > converse-in

To customize anything else, just modify the code directly and recompile. It's super simple and thoroughly documented for a reason!

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
