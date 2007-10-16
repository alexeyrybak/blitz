--TEST--
controller include method
--FILE--
<?php
include('common.inc');

$T = new Blitz();
$T->load('dummy');

$names = array('Dude', 'Sobchak', 'Donny');
$T->setGlobals(array('global' => 'hello, '));

for($i=0; $i<3; $i++) {
    echo $T->include('include-method.tpl',
        array(
            'i' => $i, 
            'block'=> array('name' => $names[$i])
        )
    );
}

?>
--EXPECT--
i = 0  hello, Dude
i = 1  hello, Sobchak
i = 2  hello, Donny
