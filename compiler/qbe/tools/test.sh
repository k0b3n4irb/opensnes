#!/bin/sh

dir=`dirname "$0"`
if [ -z "${bin:-}" ]; then
	bin=$dir/../qbe
fi
if [ -z "${binref:-}" ]; then
	binref=${bin}.ref
fi

tmp=/tmp/qbe.zzzz

drv=$tmp.c
asm=$tmp.s
asmref=$tmp.ref.s
exe=$tmp.exe
out=$tmp.out

qemu_not_needed() {
	"$@"
}

cc=
find_cc_and_qemu() {
	if [ -n "$cc" ]; then
		return
	fi
	target="$1"
	candidate_cc="$2"
	if $candidate_cc -v >/dev/null 2>&1; then
		cc=$candidate_cc
		echo "cc: $cc"

		if [ "$target" = "$(uname -m)" ]; then
			qemu=qemu_not_needed
			echo "qemu: not needed, testing native architecture"
		else
			qemu="$3"
			if $qemu -version >/dev/null 2>&1; then
				sysroot=$($candidate_cc -print-sysroot)
				if [ -n "$sysroot" ]; then
					qemu="$qemu -L $sysroot"
				fi
				echo "qemu: $qemu"
			else
				qemu=
				echo "qemu: not found"
			fi
		fi
		echo

	fi
}

init() {
	case "$TARGET" in
	arm64)
		for p in aarch64-linux-musl aarch64-linux-gnu
		do
			find_cc_and_qemu aarch64 "$p-gcc -no-pie -static" "qemu-aarch64"
		done
		if test -z "$cc" -o -z "$qemu"
		then
			echo "Cannot find arm64 compiler or qemu."
			exit 77
		fi
		bin="$bin -t arm64"
		;;
	rv64)
		for p in riscv64-linux-musl riscv64-linux-gnu
		do
			find_cc_and_qemu riscv64 "$p-gcc -no-pie -static" "qemu-riscv64"
		done
		if test -z "$cc" -o -z "$qemu"
		then
			echo "Cannot find riscv64 compiler or qemu."
			exit 77
		fi
		bin="$bin -t rv64"
		;;
	x86_64)
		for p in x86_64-linux-musl x86_64-linux-gnu
		do
			find_cc_and_qemu x86_64 "$p-gcc -no-pie -static" "qemu-x86_64"
		done
		if test -z "$cc" -o -z "$qemu"
		then
			echo "Cannot find x86_64 compiler or qemu."
			exit 77
		fi
		bin="$bin -t amd64_sysv"
		;;
	"")
		case `uname` in
		*Darwin*)
			cc="cc"
			;;
		*OpenBSD*)
			cc="cc -nopie -lpthread"
			;;
		*FreeBSD*)
			cc="cc -lpthread"
			;;
		*)
			cc="${CC:-cc}"
			ccpost="-lpthread"
			;;
		esac
		TARGET=`$bin -t?`
		;;
	*)
		echo "Unknown target '$TARGET'."
		exit 77
		;;
	esac
}

cleanup() {
	rm -f $drv $asm $exe $out
}

extract() {
	WHAT="$1"
	FILE="$2"

	awk "
		/^# >>> $WHAT/ {
			p = 1
			next
		}
		/^# <<</ {
			p = 0
		}
		p
	" $FILE \
	| sed -e 's/# //' \
	| sed -e 's/#$//'
}

once() {
	t="$1"

	if ! test -f $t
	then
		echo "invalid test file $t" >&2
		exit 1
	fi

	if
		sed -e 1q $t |
		grep "skip.* $TARGET\( .*\)\?$" \
		>/dev/null
	then
		return 0
	fi

	printf "%-45s" "$(basename $t)..."

	if ! $bin -o $asm $t
	then
		echo "[qbe fail]"
		return 1
	fi

	if test -x $binref
	then
		$binref -o $asmref $t 2>/dev/null
	fi

	extract driver $t > $drv
	extract output $t > $out

	if test -s $drv
	then
		src="$drv $asm"
	else
		src="$asm"
	fi

	if ! $cc -g -o $exe $src $ccpost
	then
		echo "[cc fail]"
		return 1
	fi

	if test -s $out
	then
		$qemu $exe a b c | diff -u - $out
		ret=$?
		reason="output"
	else
		$qemu $exe a b c
		ret=$?
		reason="returned $ret"
	fi

	if test $ret -ne 0
	then
		echo "[$reason fail]"
		return 1
	fi

	echo "[ok]"

	if test -f $asmref && ! cmp -s $asm $asmref
	then
		loc0=`wc -l $asm    | cut -d' ' -f1`
		loc1=`wc -l $asmref | cut -d' ' -f1`
		printf "    asm diff: %+d\n" $(($loc0 - $loc1))
		return 0
	fi
}

#trap cleanup TERM QUIT

init

if test -z "$1"
then
	echo "usage: tools/test.sh {all, SSAFILE}" 2>&1
	exit 1
fi

case "$1" in
"all")
	fail=0
	count=0
	for t in $dir/../test/[!_]*.ssa
	do
		once $t
		fail=`expr $fail + $?`
		count=`expr $count + 1`
	done
	if test $fail -ge 1
	then
		echo
		echo "$fail of $count tests failed!"
	else
		echo
		echo "All is fine!"
	fi
	exit $fail
	;;
*)
	once $1
	exit $?
	;;
esac
