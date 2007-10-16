--TEST--
fetch#2
--FILE--
<?php
include('common.inc');

$T = new Blitz();

$body = <<<BODY
{{ BEGIN root }}
root {{ \$title }} {{ BEGIN n1 }} n1 {{ \$dummy }}{{ END }} {{ BEGIN n2 }} n2 {{ \$dummy }}{{ END }}
{{ END }}
{{ BEGIN x }}+ this is x {{ \$x }} +{{ END }}
{{ BEGIN y }}- this is y {{ \$y }} -{{ END }}
{{ BEGIN z }}! this is z !{{ END }}
BODY;

$T->load($body);

$T->context('/root');
$T->set(array('title' => 'title1'));

$T->context('/root/n1');
$T->set(array('dummy' => 'dummy1'));

$T->context('/root/n2');
$T->set(array('dummy' => 'dummy2'));

echo $T->fetch('/root/n1');

echo $T->fetch('/root', array('title' => 'title2'));

$T->context('/x');
$T->set(array('x' => 'x1'));
$T->iterate();

$T->context('/x');
$T->set(array('x' => 'x2'));

$T->context('/y');
$T->set(array('y' => 'y1'));

//$T->dumpIterations();

$i = 0;
while($i++<5) {
    echo $T->fetch('/x');
}

echo $T->parse();

?>
--EXPECT--
 n1 dummy1
root title2   n2 dummy2
+ this is x x2 ++ this is x x1 ++ this is x  ++ this is x  ++ this is x  +

- this is y y1 -
