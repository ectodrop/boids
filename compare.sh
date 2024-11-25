#!/bin/sh
convert $1 -coalesce a-%04d.gif                         # separate frames of 1.gif
convert $2 -coalesce b-%04d.gif                         # separate frames of 2.gif
for f in a-*.gif; do convert $f ${f/a/b} +append $f; done  # append frames side-by-side
convert -loop 0 -delay 4 a-*.gif result.gif               # rejoin frames

rm -rf a-*.gif b-*.gif
