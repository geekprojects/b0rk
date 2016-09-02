
test:
	time ../../src/b0rk/b0rk tests.Expressions
	time ../../src/b0rk/b0rk tests.Mandelbrot >test.pnm
	SUM=`cksum test.pnm|cut -d' ' -f1`; if test $$SUM = 3443774409; then echo "Output OK"; else exit 1; fi;

