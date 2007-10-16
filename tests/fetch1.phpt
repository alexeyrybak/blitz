--TEST--
fetch#1
--FILE--
<?php
include('common.inc');
$T = new Blitz('fetch.tpl');

$params = array(
    'a' => 'a_value',
    'b' => 'b_value',
    'c' => 'c_value'
);

$blocks = array('x','y','z');

foreach($blocks as $i_block) {
    echo $T->fetch($i_block, $params)."\n";
}

?>
--EXPECT--
* this is x; c is not empty! *
* this is y; b=b_value *
* this is z *
