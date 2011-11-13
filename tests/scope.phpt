--TEST--
scope lookup test #1
--FILE--
<?php

include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = "
{{ BEGIN t1 }}
v1 in \"/t1\" = {{ v1 }}
{{ BEGIN t2 }}
v2 in \"/t1/t2\" = {{ a1.v2 }}, a1.v2 in \"/t1/t2\" is {{ IF a1.v2 }}NOT_EMPTY{{ END }}{{ UNLESS a1.v2 }}EMPTY{{ END }}
{{ BEGIN t3 }}
v3 in \"/t1/t2/t3\" = {{ a2.v3 }}, v1 in \"/t1/t2/t3\" {{ IF v1 }}NOT_EMPTY{{ END }}{{ UNLESS v1 }}EMPTY{{ END }}
{{ END }}
{{ END }}
{{ END }}
{{ IF v1 }}v1 in \"/\" is not empty{{ END }}
";

ini_set('blitz.scope_lookup_limit', 3);
run($body);
ini_set('blitz.scope_lookup_limit', 2);
run($body);
ini_set('blitz.scope_lookup_limit', 1);
run($body);
ini_set('blitz.scope_lookup_limit', 0);
run($body);

function run($body) {
    $T = new Blitz();
    $T->load($body);

    $T->set(
        array(
            'v1' => 'v1',
            't1' => array(
                'a1' => array(
                    'v2' => 'v2'
                ),
                't2' => array(
                    'a2' => array(
                        'v3' => 'v3'
                    ),
                    't3' => array(array()),
                ),
            )
        )
    );

    $T->display();
}

?>
--EXPECT--
v1 in "/t1" = v1
v2 in "/t1/t2" = v2, a1.v2 in "/t1/t2" is NOT_EMPTY
v3 in "/t1/t2/t3" = v3, v1 in "/t1/t2/t3" NOT_EMPTY
v1 in "/" is not empty
v1 in "/t1" = v1
v2 in "/t1/t2" = v2, a1.v2 in "/t1/t2" is NOT_EMPTY
v3 in "/t1/t2/t3" = v3, v1 in "/t1/t2/t3" EMPTY
v1 in "/" is not empty
v1 in "/t1" = v1
v2 in "/t1/t2" = v2, a1.v2 in "/t1/t2" is NOT_EMPTY
v3 in "/t1/t2/t3" = v3, v1 in "/t1/t2/t3" EMPTY
v1 in "/" is not empty
v1 in "/t1" = 
v2 in "/t1/t2" = , a1.v2 in "/t1/t2" is EMPTY
v3 in "/t1/t2/t3" = , v1 in "/t1/t2/t3" EMPTY
v1 in "/" is not empty
