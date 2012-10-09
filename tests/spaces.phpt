--TEST--
spaces inside tags
--FILE--
<?php
include('common.inc');

$T = new Blitz();
$T->load('<!--  BEGIN some  -->bla<!--  END  -->');
$T->display(array('some' => array(array())));
?>
--EXPECT--
bla
