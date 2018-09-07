# What?
A `sync_call()` util that is able to synchronize calls to a callable objects.

# How?
You'll see ;)

# Why?
Not sure.

# Before you go further
I really encourage you to read the art (: It explains how first version of `sync_call` works. It's used here.
http://stryku.pl/poetry/mutexes_are_passe.php

# Goals

1. Accept callables that return void.
2. Accept callables that return !void and return what callable returns.
3. Accept callables that takes parameters (and keep parameter types).
4. Give to user possibility to distinguish callables that need to be synced relatively to each other.
5. [DONE] Be useless.

Not much more here. See `mutexes_are_passé.cpp` for code and explanation.

See `tests.cpp` for examples.

Needs C++17.
Won't compile under gcc by default, becaues of non ascii `é`. Sorry.
