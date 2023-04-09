https://tutorcs.com
WeChat: cstutorcs
QQ: 749389476
Email: tutorcs@163.com
#!/bin/bash

BIN=./cmpsc473-p3
ANS=outputs/
OUTDIR=eval
IMG=$OUTDIR/grade-disk.img
LOG=$OUTDIR/compile.log
VERBOSE=false
TIMEOUT=5
YOUR_GRADE=0

LINE="............................"
RED="\x1B[31m"
GREEN="\x1B[32m"
RED_BG="\x1B[41m"
GREEN_BG="\x1B[42m"
END="\x1B[0m"

OKAY="${GREEN}OKAY${END}"
PASS="${GREEN}PASS${END}"
FAIL="${RED}FAIL${END}"

# Grades
declare -A GRADES=(
	# Successful compilation - 0pt.
	[Compiles]=0

	# Task 1 - 3pts.
	[inputs/task1a.cmd]=0.5
	[inputs/task1b.cmd]=2
	[inputs/task1b-err.cmd]=0.5

	# Task 2 - 1pt.
	[inputs/task2.cmd]=1

	# Task 3 - 3pts.
	[inputs/task3a.cmd]=.75
	[inputs/task3a-err.cmd]=.25
	[inputs/task3b.cmd]=.75
	[inputs/task3b-err.cmd]=.25
	[inputs/task3c.cmd]=.75
	[inputs/task3c-err.cmd]=.25

	# Task 4 - 2pts.
	[inputs/task4a.cmd]=.2
	[inputs/task4a-err.cmd]=.4
	[inputs/task4b.cmd]=.2
	[inputs/task4b-err.cmd]=.5
	[inputs/task4c.cmd]=.2
	[inputs/task4c-err.cmd]=.5
)

function usage()
{
	echo -e "${RED}usage:${END} $0 [-v]"
	echo -e "  -v ........ verbose"
	echo -e "  -h ........ help"
}

function setup()
{
	# Remove stale files
	rm -f  "$IMG"
	rm -rf "$OUTDIR"

	# Create directory for output
	mkdir  "$OUTDIR"
}

function log_title()
{
	printf "%s\n" "$1"           >>"$LOG"
}

function log_error()
{
	echo -ne "ERROR: $@\n"       >>"$LOG"
}

function log_grade()
{
	echo -ne "\nYour grade = ${YOUR_GRADE} (out of 9.00)\n" | tee -a "$LOG"
}

function report()
{
	printf "%s %s [ $2 ]\n" "$1" "${LINE:${#1}}"
}

function grade_compile()
{
	log_title "Compiling"

	# Do not proceed if compilation fails
	make | tee -a $LOG
	if [ ${PIPESTATUS[0]} -ne 0 ]; then
		log_grade
		log_error "Failed to compile. Exiting."
		exit -1
	fi

	# Update grade
	report "Compiles" "$OKAY"
	YOUR_GRADE=$(echo $YOUR_GRADE + ${GRADES[Compiles]} | bc)
}

function grade_input()
{
	log_title "Running $1"
	outfile="$OUTDIR/$(basename $1 .cmd).log"
	ansfile="$ANS/$(basename $1 .cmd).log"

	# Run
	timeout $TIMEOUT "$BIN" "$IMG" $1 >$outfile 2>&1

	# Verify
	diffout=$(diff "$ansfile" "$outfile")
	echo "$diffout" >> "$LOG"
	if [ ${#diffout} -ne 0 ]; then
		report "$1" "$FAIL"
		$VERBOSE && echo "$diffout" 
		return
	fi

	# Update grade
	report "$1" "$PASS"
	YOUR_GRADE=$(echo $YOUR_GRADE + ${GRADES[$1]} | bc)
}

# Get the options
while getopts ":hv" option; do
   case $option in
      v) # verbose
         VERBOSE=true
         ;;
    h|*) # help
         usage
         exit;;
   esac
done
shift $((OPTIND-1))

# Ensure no spurious arguments
if [ ! -z "$*" ]; then
   usage
   exit -1
fi

# START GRADING
setup
echo "Generated on `date`" >$LOG

grade_compile
grade_input   "inputs/task1a.cmd"
grade_input   "inputs/task1b.cmd"
grade_input   "inputs/task1b-err.cmd"
grade_input   "inputs/task2.cmd"
grade_input   "inputs/task3a.cmd"
grade_input   "inputs/task3a-err.cmd"
grade_input   "inputs/task3b.cmd"
grade_input   "inputs/task3b-err.cmd"
grade_input   "inputs/task3c.cmd"
grade_input   "inputs/task3c-err.cmd"
grade_input   "inputs/task4a.cmd"
grade_input   "inputs/task4a-err.cmd"
grade_input   "inputs/task4b.cmd"
grade_input   "inputs/task4b-err.cmd"
grade_input   "inputs/task4c.cmd"
grade_input   "inputs/task4c-err.cmd"

log_grade
exit 0
