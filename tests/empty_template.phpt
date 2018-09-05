--TEST--
empty template test 
--FILE--
<?php

$t = new Blitz(__DIR__."/empty.tpl");

var_dump($t->parse());
var_dump($t->fetch("/"));
var_dump($t->display());
var_dump($t->getTokens());
var_dump($t->iterate('/main'));
var_dump($t->context('/main/first'));
var_dump($t->block('5588/lklk'));
var_dump($t->iterate('5588/lklk'));

$t = new Blitz();

var_dump($t->parse());
var_dump($t->fetch("/"));
var_dump($t->display());
var_dump($t->getTokens());
var_dump($t->iterate('/main'));
var_dump($t->context('/main/first'));
var_dump($t->block('5588/lklk'));
var_dump($t->iterate('5588/lklk'));

?>
--EXPECTF--
string(0) ""
string(0) ""
string(0) ""
array(0) {
}
bool(true)
string(1) "/"
bool(true)
bool(true)
string(0) ""
string(0) ""
string(0) ""
array(0) {
}
bool(true)
string(1) "/"
bool(true)
bool(true)
