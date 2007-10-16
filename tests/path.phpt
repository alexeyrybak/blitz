--TEST--
relative path (blitz.path)
--FILE--
<?php

include('common.inc'); // path set is here already

$T = new Blitz('var.tpl');
echo $T->parse(array('a'=>'Dude', 'b'=>'Sobchak', 'c'=>'Donny'));

?>
--EXPECT--
Dude
Sobchak
Donny
