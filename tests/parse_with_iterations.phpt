--TEST--
parse with iteration set 
--FILE--
<?php
include('common.inc');

$T = new Blitz();
$T->load('<!-- BEGIN test -->({{ $i }}) hello, {{ $name }}'."\n".'<!-- END test -->');
$vars = array(
    'test' => array(
        array('name' => 'Dude', 'i' => 1),
        array('name' => 'Sobchak', 'i' => 2),
        array('name' => 'Donny', 'i' => 3),
    )
);

echo $T->parse($vars);

?>
--EXPECT--
(1) hello, Dude
(2) hello, Sobchak
(3) hello, Donny
