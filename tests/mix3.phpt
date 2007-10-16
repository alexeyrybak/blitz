--TEST--
mix #3
--FILE--
<?php
include('common.inc');

$body = "start = {{\$start }}\n{{ begin test }}name = {{ \$name }}\n{{ end }}";

$T = new Blitz();
$T->load($body);
$T->iterate("/test");
$T->context("/test");
$T->set(array("name" => "john"));
$T->context("/");
$T->set(array("start" => "yes"));

echo $T->parse();

?>
--EXPECT--
start = yes
name = john
