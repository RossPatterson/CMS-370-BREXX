# bREXX Internals

**Note: As of 2026-03-25, this was accurate.  After that, who knows?**

[CMS-370-BREXX](https://github.com/RossPatterson/CMS-370-BREXX) is a port of
[Vasilis Vlachoudis's bREXX](https://github.com/vlachoudis/brexx) Rexx
implementation, ported to the CMS component of VM/370 (_e.g._, the [VM/370
Community Edition](http://vm370.org/vm)).  You can read  Vlachoudis's original
[README.md](READMEVV.md) for his explanation of bREXX, as it was originally
written for MS-DOS, and as it grew to support Unix, Linux, and MS-Windows.
There is also a [port to MVS](https://github.com/mvslovers/brexx370), but
despite hints in the source, this port is separate from it.

## Code Structure

bREXX is composed of 5 main components:
* the _[main program](#main-program)_, which is the entrypoint from CMS
* the _[compiler](#compiler)_, which compiles Rexx source code to a
bREXX-specific pseudo-instruction set
* the _[interpreter](#interpreter)_, which executes the pseudo-instructions
* the _[Libstring library](#libstring-library)_, which handles all Rexx data
and performs operations upon them
* the _[built-in functions](#built-in-functions), which perform all the
pre-defined functions

### Main Program

CMS calls the _main program_ to begin executing a Rexx program.  The _main
program_ reads the Rexx program from disk or storage, and stores it as a single
large `Lstr` object for compilation.  It then calls the
_[compiler](#compiler)_, and if there are no errors, the
_[interpreter](#interpreter)_, and then exits with the exit code of the Rexx
program.

The _main program_ is composed primarily of the following source files:
* [`main.c`](main.c), the entrypoint from CMS, which does some simple setup and
then invokes `rexx.c:RxRun()` to do the real work, and upon return, tears down
the setup.
* [`rexx.c`](rexx.c), which does some more-in-depth initialization of the bREXX
environment, reads the Rexx source code, and then invokes the
_[compiler](#compiler)_ and the _[interpreter](#interpreter)_.

### Compiler

The _[main program](#main-program)_ calls the _compiler_ to compile the Rexx
program, and the _[interpreter](#interpreter)_ calls it to compile strings that
are supplied to the `INTERPRET` instruction.  Its entrypoint is
`compile.c:RxCompile()`.  The output of the compiler is an array in memory
of pseudo-instructions implementing the Rexx code, which the
_[interpreter](#interpreter)_ can later execute.

The _compiler_ is a recursive-descent compiler, with an integrated just-in-time
tokenizer.  It is roughly a LALR(1) compiler, and was written by hand, not
generated from a Rexx language grammar.

The _compiler_ is is composed primarily of the following
source files:
* [`compile.c`](compile.c), which contains the entrypoint to compile some Rexx
code, as well as many `C_whatever()` functions that compile specific Rexx
instructions.
* [`expr.c`](expr.c), which compiles expressions.
* [`nextsymb.c`](nextsymb.c), which tokenizes the input Rexx program.
* [`template.c`](template.c), which compiles `PARSE` templates.

### Interpreter

The _[main program](#main-program)_ calls the _interpreter_ to execute a
Rexx program after it is compiled by the _[compiler](#compiler)_.  It receives
an array of instructions produced by the _[compiler](#compiler)_.  Its
entrypoint is `_interpre.c:RxInterpret()`.

The _interpreter_ uses the _[stack](#stack)_ and its companion
_[temporary variables](#temporary-variables)_ to hold values, represented as
[`Lstr`](#lstr) objects, to perform all the computations of the Rexx program.

The _interpreter_ is is composed primarily of the following source files:
* [`bintree.c`](bintree.c), which handles the binary tree structures that store
Rexx symbols.
* [`interpre.c`](interpre.c), which executes the compiled code.
* [`stack.c`](stack.c), which implements the [_console stack_](#console-stack)
on other platforms, but is unused in CMS.
* [`trace.c`](trace.c), which traces program execution.

### Libstring library

The _Libstring library_ is a collection of C routines that manipulate Rexx
values, which are stored as `LStr` objects, and which therefore may be C
`long`, `double`, or `char` array variables.

The _Libstring library is is composed primarily of the following source files:
* [`lstring.c`](lstring.c), which implements the various `LStr` primitive
operations.
* [`lstring.h`](lstring.h), which defines the `LStr` C structure and many
`Lwhatever()` pre-processor macros to access the contents of `LStr` objects.


### Built-in Functions

The _built-in functions_ are a collection of C routines that implement the
various pre-defined functions of the standard Rexx library.  They are called by
the _[interpreter](#interpreter)_ to do their task, receiving their parameters
via the `Args` structure, and returning their result in it.  The
[`RexxFuncs.c`](RexxFuncs.c) file provides the list of functions, and the
[`builtin.c`](builtin.c) file implements them through a set of many-to-one
shims that handle the parameters, and then, in most cases, hand off processing
to a specific routine in another file.

The shim functions in `builtin.c` are named according to the parameter
patterns they handle.  For example, `R_O()` handles the group of _built-in
functions_ that take no parameters.  `R_C()` handles those that take one
optional single-character parameter.  `R_oSoS()` handles those that take two
optional string parameters.  `R_SoSoS()` handles those that take a required
string parameter, followed by two optional string parameters.

Many of the C files in the bREXX source code are part of this collection - most
implement just a single _built-in function_, and are named as an up-to-8-letter
abbreviation of their function name.

## Major data structures

### Context

In all other bREXX implementations, the operating system creates a separate
process for "inner" EXECs, but CMS does not, and therefore it would share
global `static` variables among all the running EXECs, with negative results.
The _context_ was added to bREXX for CMS to keep those variables separate.
All the `static` variables (or at least, all that are not `#if`-ed out for CMS)
were moved into the new [`Context` structure](context.h), which is created by
[`context.c:InitContext()`](context.c), and is destroyed by the GCC C-library
code during program termination.  All references to `static` variables were
changed from "`whatever`" to "`(context->location_whatever)`" (where _location_
is a word derived from the variable's original filename), and all routines that
use them were changed to set the context pointer, `context`, from the C-library
`CMSGetPG()` function.

While bREXX cannot yet run from a Discontiguous Shared Segment (DCSS), the
_context_ structure also contributes to the goal of making that possible.  VM
does not allow a program to modify memory in a DCSS, which makes read/write
`static` variables impossible.  As of this writing, there are still a few such
variables in bREXX, but there is a long-term goal to eliminate them and load
bREXX into a DCSS.

### Stack
The _data stack_ (or _stack_), [`(context->interpre_RxStck)`](context.h),
is a fixed-size array of pointers to [`Lstr`](#lstr) objects.  The stack is
used starting at`interpre_RxStck[0]`.  It is created during initialization of
the `interpreter`, in [`interpre.c:RxInitInterpret()`](interpre.c), and is
destroyed during its termination, in
[`interpre.c:RxDoneInterpret()`](interpre.c).  The _stack top pointer_ (_i.e._,
highest in-use array index) is `context->interpre_RxStckTop`.  The stack size
is set from the [`STACK_SIZE` constant](rexx.h).

If bREXX is compiled with the `DEBUG` C pre-processor symbol defined,
`RxDoneInterpret()` will display the remaining stack contents (`_i.e._`, the
elements from 0 to the `stack top`).  While this may be an indication of the
_interpreter_ failing to manage the stack pointer correctly, it is also the
case that Rexx programs may terminate in ways that leave data in the stack.

### Temporary variables
Temporary variables are stored in the array
[`context->interpre__tmpstr`](context.h), which parallels the [stack](#stack).
Each stack item (_i.e._, `(context->interpre_RxStck)[n]`) has a dedicated
temporary variable, `(context->interpre__tmpstr)[n]`, which is a pre-allocated
[`Lstr`](#lstr) object.  These temporary variables are used for a variety of
purposes, most often for holding the intermediate and final values of
expressions.  The array is created during initialization of the
[_interpreter_](#interpreter), in `interpre.c:RxInitInterpret()`, and is
destroyed during its termination, in `interpre.c:RxDoneInterpret()`.  Unlike
the _stack_, there is no separate top pointer, as each item should only be
accessed as part of its corresponding stack item.  Like the _stack_, this
array's size is set from the `STCK_SIZE` constant.

### Lstr

The _Lstr_, or "Libstring" structure, carries a value in bREXX.  The actual
data may be a C `long`, `double`, or an array of `char` with a length (and
without the trailing `'\0'` that a C string would have).  Almost all of bREXX
deals in manipulating and passing around `Lstr` values, rather than bare C
values.  Likewise, the [stack](#stack) and
[temporary variables](#temporary-variables) are arrays of pointers to `Lstr`
objects.  The _[Libstring library](#libstring-library)_ provides a wide variety
of routines to manipulate `Lstr`s and convert them from one datatype to
another.

### Console Stack

Rexx started on VM/SP CMS systems, where there is a console-input stack (often
just referred to as "the stack"), and which already had a history of use to
pass output of one command as input to another.  Rexx has several built-in
functions and instructions to operate on that data - `Queued()`, `PUSH`,
`PULL`, and `QUEUE`.  On other platforms, bREXX implements a similar data
stack, but on CMS, it uses the CMS OS's native stack.

## Build Process

bREXX for CMS is built entirely on CMS, from source code stored in CMS files.
The `BRXBUILD EXEC` procedure runs that process from start to finish, using
other `BRXwhatever EXEC`s as building blocks.  It is delivered for VM/370
Community Edition as the `MAINTC` userid and the `GCCBRX` simulated DASD
volume.

The `MAINTC` userid is organized as follows, with all the minidisks being on
the `GCCBRX` simulated DASD volume:
* 191 - a work minidisk
* x94 - a set of minidisks for the native-CMS "GCCLIB" C standard library.
* x95 - a set of minidisks for the OS Simulation "PDPCLIB" C standard library.
* 193 - the output minidisk for BRXBUILD without DEBUG defined
* 293 - the output minidisk for BRXBUILD with DEBUG defined
* 393 - the bREXX source code files
* 493 - an old minidisk, possibly an old version of GCCLIB.
* 19C - a set of tools for building bREXX, GCCLIB, and PDPCLIB.

See [`TOOLDISK MEMO`](tools/tooldisk.memo) and the various `* HELPCMD` files
for more details on the tools used by `BRXBUILD`.

### File lists

* tools/brxhelp.exec - all the test files that go on the `MAINT 19D` HELP
minidisk.
* tools/brxtools.exec - all the tools that go on the `MAINTC 19C` minidisk.
* tools/brxtests.exec - all the test files.

### GitHub Build System

There is an automated build system for the
[CMS-370-BREXX](https://github.com/RossPatterson/CMS-370-BREXX) Git repository,
which launches VM/370 Community Edition in a Docker container, runs `BRXBUILD`,
and packages the results as VMARC files, AWSTAPE simulated tape files, and a
simulated 3350 DASD.  It operates through the GitHub Actions system, and is
comprised of the following files:
* .github/workflows/build.yml - the driver that GitHubg uses to launch the
build
* cmsbuild.sh - a Linux shell script used by `build.yml` to import the source,
run `BRXBUILD`, and export the results.
* cmsinst.sh - a Linux shell script used by `build.yml` to test the various
ways to install bREXX from the `BRXBUILD` results.


### Releasing

To create a new release of bREXX for CMS, update the release number (_i.e._,
`#define CMS_VERSION x.y.z`) in `config.h`, compile `interpre.c` and `main.c`,
and verify the result (_e.g._, `DMSREX VERSION` and `PARSE VERSION`).  When
all is correct, push the code to GitHub, then tag it as "`vx.y.z".  The build
system will recognize tags beginning with "v" and create a release in GitHub.

bREXX release numbers follow the [Semantic Versioning](https://semver.org/)
model (_i.e._, "_major.minor.patch_").  For bREXX purposes, we define a _patch
release_ (_i.e._ `x.y.z` where z >= 1) as containing just bugfixes, a _minor
release_ (_i.e._, `x.y.0`) as containing at least one non-breaking change that
is not a bugfix, and a _major release_ (_i.e._ `x.0.0`) as containing at least
one breaking change.
