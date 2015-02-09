--TEST--
conditional with predefined constants
--FILE--
<?php

include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
{{ BEGIN some }}{{ IF _first }}<| {{ END }}{{ IF _even }}:{{ ELSE }}_{{ END }}{{ IF _odd }}={{ ELSE }}-{{ END }}({{ _num }}){{ item }}{{ UNLESS _last }} , {{ END }}{{ IF _last }}  |>{{ END }}{{ END }}
{{ IF foo.bar }}foobar{{ ELSE }}nofoobar{{ END }}
{{ IF foo }}foo{{ ELSE }}nofoo{{ END }}
{{ IF some }}some{{ ELSE }}nosome{{ END }}
{{ IF great.stuff }}greatstuff: {{ great.stuff.item }}{{ ELSE }}nogreatstuff{{ END }}
BODY;

$T = new Blitz();
$T->load($body);

$data = array(
    'some' => array(
        array('item' => 'must'),
        array('item' => 'love'),
        array('item' => 'dogs')
    ),
    'great' => array(
        'stuff' => array('item' => 'wow')
    )
);

$T->display($data);

?>
--EXPECT--
<| :-(1)must , _=(2)love , :-(3)dogs  |>
nofoobar
nofoo
some
greatstuff: wow
