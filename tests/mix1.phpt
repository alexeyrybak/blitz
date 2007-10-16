--TEST--
mix #1
--FILE--
<?php
include('common.inc');

$body = "start = {{\$start }}\n{{ begin test }}name = {{ \$name }}\n{{ end }}";
$T = new Blitz();
$T->load($body);
$T->iterate("/test");

$T->set(array("name" => "john")); // this will set name to root context and it will not be seen

$T->context("/");
$T->set(array("start" => "done"));

echo $T->parse();

?>
--EXPECT--
start = done
name =
