# Author.: Ole J
# Date...: 23.03.2008
# License: Whatever

# Wraps a completion function
# make-completion-wrapper <actual completion function> <name of new func.> <name to replace with command>
#                         <command name> <list supplied arguments>
# eg.
# 	alias agi='apt-get install'
# 	make-completion-wrapper _apt_get _apt_get_install agi apt-get install
# defines a function called _apt_get_install (that's $2) that will complete
# the 'agi' alias. (complete -F _apt_get_install agi)
#


_completion_loader apt-get

function make-completion-wrapper () {
	local function_name="$2"
	local arg_count=$(($#-4))
	local comp_function_name="$1"
	local replacename="$3"
	shift 3
	local realname="$@"
	local function="
function $function_name {
	((COMP_CWORD+=$arg_count))
	COMP_WORDS=( "$@" \${COMP_WORDS[@]:1} )
	COMP_LINE=\${COMP_LINE/"$replacename"/'"$@"'}
	((COMP_POINT+=${#realname}))
	((COMP_POINT-=${#replacename}))
	"$comp_function_name"
	
	return 0
}"
	eval "$function"
	echo $function_name
	echo "$function"

}
