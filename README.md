Ruby/Cares
==========

##Introduction

Ruby/Cares is a C extension for the
[c-ares](http://daniel.haxx.se/projects/c-ares/) library. It provides an
asynchronous DNS resolver to be used with ruby scripts.


##Requirements

Ruby/Cares currently works with Ruby 1.9.2 and does not have any external
dependencies (c-ares 1.7.5 is included in the distribution). It has been tested
on Mac OS X.

##Installation

From rubygems:

```
$ gem install ruby-cares
```

From source:
```
$ bundle install
$ rake compile    # for development
$ rake install    # or you can build and install the gem
```

##License

Copyright (c) 2011 David Albert <davidbalbert@gmail.com>
Copyright (c) 2006 Andre Nathan <andre@digirati.com.br>

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.


##Authors

Ruby/Cares was originally developed by Andre Nathan. This fork has been updated
to include c-ares 1.7.5 included in the gem and to support Ruby 1.9.2 by David
Albert.
