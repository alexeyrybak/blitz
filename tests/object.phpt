--TEST--
testing for objects set
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$T = new Blitz();

$body = <<<BODY
{{ IF object }}
object is set and not empty
{{ ELSE }}
object not set or empty
{{ END object }}
===================================================================

BODY;

$T->load($body);

$object = new stdClass();
$data = array(
    'object' => $object
);
$T->display($data);

$data = array();
$T->display($data);

$object->key = 'value';
$data = array(
    'object' => $object
);
$T->display($data);

?>
--EXPECT--
object not set or empty
===================================================================
object not set or empty
===================================================================
object is set and not empty
===================================================================
