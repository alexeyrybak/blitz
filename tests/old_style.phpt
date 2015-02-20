--TEST--
complex conditions
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
{{ IF AAA }}AAA{{ END }}
<!-- IF BBB -->BBB<!-- END -->
###

BODY;

$T = new Blitz();
$T->load($body);
$T->display();
$T->display(array('AAA' => true, 'BBB' => true));

?>
--EXPECT--
###
AAA
BBB
###
