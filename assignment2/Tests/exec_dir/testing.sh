#!/bin/bash

EXEC=$1
VAULT_FILE=$2
FILES_DIRECTORY="../files"


containsElement () {
  local e
  for e in "${@:2}"; do [[ "$e" == "$1" ]] && return 0; done
  return 1
}


addAndRemoveRandomFiles () {
    COUNTER=1
    num_of_files=$(ls -1q $FILES_DIRECTORY | wc -l)
    random_indexes=$(shuf -i 1-$num_of_files -n 120)
    for file in $FILES_DIRECTORY/*.txt
    do
	containsElement $COUNTER $random_indexes
	if [ $? -eq 0 ]
	then
            $EXEC $VAULT_FILE add $file >> /tmp/${EXEC##*/}.log  2>&1
	fi
	let COUNTER+=1
    done

    COUNTER=1
    for file in $FILES_DIRECTORY/*.txt
    do
	if [ $[ $RANDOM % 2 ] -eq 0 ]
	then
	    containsElement $COUNTER $random_indexes
	    if [ $? -eq 0 ]
	    then
                $EXEC $VAULT_FILE rm ${file##*/} >> /tmp/${EXEC##*/}.log  2>&1
	    fi
	fi
	let COUNTER+=1
    done
}

differBetweenFetchedAndOrigin () {
    for file in $FILES_DIRECTORY/*.txt
    do
        $EXEC $VAULT_FILE fetch ${file##*/} >>  /tmp/${EXEC##*/}.log 2>&1
	if [ $? -eq 0 ]
	then
            assertNull $(diff $file ./${file##*/})
	    if [ $? -eq 1 ]
            then
	    	echo "$(diff $file ./${file##*/})"
	    fi
	    if [ $? -eq 1 ]
	    then 
		echo ${file##*/}
	    fi
	fi
    done
}


testMaxFilesAndSpace () {
    
    $EXEC $VAULT_FILE init 2M >>  /tmp/${EXEC##*/}.log 2>&1
    
    for file in $FILES_DIRECTORY/*.txt
    do
        $EXEC $VAULT_FILE add $file >>  /tmp/${EXEC##*/}.log 2>&1
    done
}


testFetch () {
    COUNTER=0

    $EXEC $VAULT_FILE init 50M >>  /tmp/${EXEC##*/}.log 2>&1

    for file in $FILES_DIRECTORY/*.txt
    do
        $EXEC $VAULT_FILE add $file >>  /tmp/${EXEC##*/}.log 2>&1
	res=$?
        if [ $COUNTER -gt 99 ]
        then
    	assertNotSame "0" $res
	break
        fi
	$EXEC $VAULT_FILE fetch ${file##*/} >>  /tmp/${EXEC##*/}.log 2>&1
	fetchedFile=$file
        let COUNTER+=1
    done

    assertNull $(diff $fetchedFile ./${fetchedFile##*/})
}


testMultipleRemoves () {
    COUNTER=0
    failed=0

    $EXEC $VAULT_FILE init 5M >>  /tmp/${EXEC##*/}.log 2>&1

    for file in $FILES_DIRECTORY/*.txt
    do
	break
    done
    
    while [ $COUNTER -lt 100 ]
    do
        $EXEC $VAULT_FILE add $file >>  /tmp/${EXEC##*/}.log 2>&1
	failed=$failed || $?
	$EXEC $VAULT_FILE rm ${file##*/} >>  /tmp/${EXEC##*/}.log 2>&1
	failed=$failed || $?
	let COUNTER+=1
    done
    

    $EXEC $VAULT_FILE add $file >>  /tmp/${EXEC##*/}.log 2>&1
    $EXEC $VAULT_FILE fetch ${file##*/} >>  /tmp/${EXEC##*/}.log 2>&1
    assertEquals 0 $failed
    assertNull $(diff $file ./${file##*/})
}


testDefrag() {
    $EXEC $VAULT_FILE init 30M >>  /tmp/${EXEC##*/}.log 2>&1
    for i in $(seq 1 4)
    do
        addAndRemoveRandomFiles
    done
    $EXEC $VAULT_FILE defrag >>  /tmp/${EXEC##*/}.log 2>&1
    assertEquals 0 $?
    differBetweenFetchedAndOrigin
}


testAddAndRemove() {
    $EXEC $VAULT_FILE init 30m >>  /tmp/${EXEC##*/}.log 2>&1
    for i in $(seq 1 4)
    do
        addAndRemoveRandomFiles
    done
    differBetweenFetchedAndOrigin
}

. ../shunit2/source/2.0/src/shell/shunit2
