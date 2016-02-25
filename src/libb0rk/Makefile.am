
SUBDIRS=packages

AM_CPPFLAGS = -DDATA_PATH=\"$(pkgdatadir)\"

lib_LTLIBRARIES=libb0rk.la

libb0rk_la_SOURCES=assembler.cpp context.cpp scriptfunction.cpp value.cpp class.cpp executor.cpp \
                   function.cpp parser.cpp codeblock.cpp expression.cpp lexer.cpp runtime.cpp disassembler.cpp utils.cpp
libb0rk_la_LIBADD=packages/system/libsystem.la

