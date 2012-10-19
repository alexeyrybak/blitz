--TEST--
Escape for non-UTF encoding (cp1251)
--FILE--
<?php
include('common.inc');
$string_var = 'строка в кодировке windows-1251';
$body = "<span>{{ escape(\$string_var); }}</span>";

$T = new Blitz();
$T->load($body);

$T->display(array('string_var' => $string_var));
?>
--EXPECT--
<span>строка в кодировке windows-1251</span>

