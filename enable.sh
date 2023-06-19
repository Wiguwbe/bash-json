# source this file

# builtins to load
BUILTINS=(
	jload
	jprint
	jhandler

	jnew
	jtype
	jget
	jset
	jdel

	jlen
	jcmp

	jkeys
	jvalues
	jhaskey
	jhasval
)

# where the SO is
SO_LOCATION="$HOME/bash-json/bash-json.so"

enable -f ${SO_LOCATION} ${BUILTINS[@]}
