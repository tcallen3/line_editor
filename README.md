This program is a simple but functional line-based text editor, which can
insert, copy, and transform text in a variety of ways. It is inspired by
tools like Unix `ed` and `lsedit` from the MUCK world. 

While alien by modern standards of editing, it demonstrates how much is
possible without a GUI of any kind, and there are still some niche applications
for those operating on remote shells without things like X-forwarding.

The general command reference and usage guide follows:

Any line not starting with '.' is inserted at the current line.
Lines starting with '..', '."', or '.:' are added with the '.' removed.

In the usage below, arguments in '[]' are optional.
Abbreviations: sl - start line, el - end line, dl - destination line.
Relative line references: ^ - first line, . - current line, $ - last line

| Command | Description |
----------|-------------|
.end      | Exits after saving changes.
.abort    | Exits without saving.
.h        | Displays this help.
.l [sl [el]] | Lists lines in the range provided (empty = list all).
.p [sl [el]] | Like '.l' but prints line numbers.
.del [sl [el]] | Deletes the given lines (empty = current).
.copy [sl [el]]=dl | Copies selected lines to destination.
.move [sl [el]]=dl | Moves selected lines to destination.
.find [sl]=text    | Searches for given text starting at specified line.
.replace [sl [el]]=/old/new/ | Replaces old text with new in given range.
.left [sl [el]]    | Align text in range to the left.
.center [sl [el]]=cols | Center text in range to given column width.
.right [sl [el]]=cols  | Align text in range to right for given column width.
