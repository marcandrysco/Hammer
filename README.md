Hammer BS
=========

When all you have is a hammer.


## Rationale

When I start a software project, I find selecting a build system (BS) to be
extremely unpleasant but very important. What BS do I use? I have walked the
halls of CMake, fought the dragons of Autotools, and made the pilgramages to
GNU Make. Each one is more ham-fisted than the last. At my journeys end, I
feel unsatisfied. My heart aches. An emptiness remains, and I, like any other
proud computer scientist, resolve to correct this wrong in the only way I know
how: starting yet another open source project to solves the same exact
problem.


## Requirements

Adding Hammer BS to your project only requires the hammer script
(`hammer.sh`). Once added, you and your users only require a couple of items:

Users of your project require less:

  * Some type of POSIX compliant shell (i.e. Bash or Dash).
  * A C compiler (i.e. gcc).


## Initializing a Project

Just run `./hammer.sh init`. Seriously.


## Upgrading a Project

Replace `hammer.sh` with an updated copy, then run `./hammer.sh init`. This
isn't so hard.


## Writing your `hamm`er file

Okay, this is where the real meat is. The most basic component of a `hamm`er
file are rules that say three things: what files are going to be created
(targets), what files are required (dependencies), and what commands we will
run to create the targets from the depencencies (called just commands). Let's
look at a rule that take two files called `a` and `b` to create a combined
file called `both`.

    both : a b {
      cat a b > both ;
    }

To break it down, the first line gives the targets (`both`), followed by a
color (`:`), followed by dependencies (`a b`). Afterward, the open brace (`{`)
indicates a list of commands. Each command ends with a semicolor (`;`), so the
command to run is `cat a b > both`. The close brace (`}`) indicates the end of
the command list.

To test this out, you can create two files `a` and `b`. Try running `echo
'first file' > a` and `echo 'second file' > b`. Now, running `make` should
generate a file `both` containing both lines. You will also see the command
printed to the terminal to verify it has been run. When running `make` again,
you won't see anything executed because nothing has been updated. Simply edit
one file to see `make` update the output file `both`.


### Special Targets

Targets that begin with a dot (`.`) are called special targets. They do not
reference a file path; instead, they are always built. If you have a file that
starts with a dot, just put the file in quotes (e.g. `".not_special"`).


## File Structure

There are three types of tokens: keywords, symbols, and strings. Keywords are
unquoted strings such as `if`. Symbols are any of the symbols such as `=`.
Strings are any quoted string, either double or single quotes, or an unquoted
text containing only letters, numbers, periods, underscores, and dashes. To
use a keywrod in a string, place it within a quote.

Every hammer file consists of a series of statements. Each statement is a list
of strings or symbols ending in either a `;`, denoting the end of a statement,
or a `{` indicate a list of substatements ending with `}`. The type of
statement is determined either by a beginning keyword (e.g. `if`) or a symbol
(e.g. `=`). Any keyword or symbol can be interpretted as a string by
surrounding it in quotes.


## Examples

### Building a C project with two source files.

```hamm
.all : prog ;

prog : a.o b.o {
  gcc $^ -o $@ ;
}

%.o : %.c {
  gcc -c $^ -o $@ ;
}

for src in $src {
  obj = $src.ext(.o)

  $obj : $src {
  }
}
```
