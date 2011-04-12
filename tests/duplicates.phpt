--TEST--
context duplicates warinings
--FILE--
<?php

include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);
ini_set('blitz.warn_context_duplicates', 1);

$body = <<<BODY
{{ BEGIN some }}
    some began
    {{ IF bla }}
        <if>
        {{ BEGIN bla }}
            item = {{ item }}
        {{ END }}
        {{ BEGIN bla }}
            item = {{ item }}
        {{ END }}
        </if>
    {{ END }}
    {{ IF bla }} bla again {{ END }}
    {{ UNLESS dummy }}
    unless worked: {{ var }}
    {{ END }}
    {{ UNLESS dummy }}
    unless worked: {{ var }}
    {{ END }}
    some ended
{{ END }}
{{ BEGIN some }}
    some again
{{ END }}
BODY;

$T = new Blitz();
$T->load($body);

?>
--EXPECT--
context name "/some/IF/bla/" duplicate in noname_loaded_from_zval
context name "/some/" duplicate in noname_loaded_from_zval
