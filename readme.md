converse
========

A command-line IM chat-like front end to absolutely anything.

`usage: converse input_file output_file`

Converse was written to play nice with apps that adhere to the Unix philosophy like [ii](http://tools.suckless.org/ii/) or [ratox](http://ratox.2f30.org/). `input_file` can for example be a FIFO into which you funnel any text you like. These are pushed to converse's message log. Any text you input in converse's bottom text input area is appened to `output_file`.

With the magic of scripting and converse, you could for example log and reply to tweets, or chat with Cleverbot, or *converse* with anything that will let you, through a single interface!

Converse is a tiny, single-file program written in C. It has two dependencies: ncurses (for a text-based UI in your terminal) and pthread (for multithreading). Both are already installed most places. To compile, all you need to do is run `make`. To customize anything, just modify the code directly and recompile. It's super simple for a reason!
