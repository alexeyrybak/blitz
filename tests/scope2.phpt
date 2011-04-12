--TEST--
scope lookup test #2
--FILE--
<?

$T = new Blitz();

$body = <<<BODY
{{ BEGIN items }}
flat variable lookup: {{ \$upper }}
{{ BEGIN inner }}
1-level lookup: {{ \$upper }}
method if with 1-level lookup: {{ if(\$upper, 'OK', 'FAILED') }}
context if with 1-level lookup: {{ IF \$upper }}OK{{ ELSE }}FAILED{{ END }}
{{ END inner }}
{{ END items }}
===================================================================

BODY;

$T->load($body);

$data = array(
    'items' => array(
        'upper' => 'OK',
        'inner' => array(
            'value' => true
        )
    )
);

ini_set('blitz.scope_lookup_limit', 0);
$T->display($data);
ini_set('blitz.scope_lookup_limit', 1);
$T->display($data);

?>
--EXPECT--
flat variable lookup: OK
1-level lookup: 
method if with 1-level lookup: FAILED
context if with 1-level lookup: FAILED
===================================================================
flat variable lookup: OK
1-level lookup: OK
method if with 1-level lookup: OK
context if with 1-level lookup: OK
===================================================================
