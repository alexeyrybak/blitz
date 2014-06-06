--TEST--
auto-escaping variables (HTML-filtering). NOTE: works 100% only for PHP > 5.4.0!
--FILE--
<?php
include('common.inc');

ini_set('default_charset', 'UTF-8');
ini_set('blitz.auto_escape', 1);

$T = new Blitz();
$T->load("{{ \$a }} {{ \$b }} {{ \$c }} {{ \$d }}");

$data = array(
    'a' => "this is 'a' \"test\"",
    'b' => '<a href="null">',
    'c' => '<>@#$%^&*()_=\'"',
    'd' => chr(192).chr(188)
);

$T->display($data);
$T->clean();

$T->setGlobals($data);
$T->display();
echo "\n";
unset($T);

$data = array(
    array('escape' => 1, 'filter' => '',        'add_prefix' => 1),
    array('escape' => 1, 'filter' => 'raw',     'add_prefix' => 1),
    array('escape' => 1, 'filter' => 'escape',  'add_prefix' => 1),
    array('escape' => 0, 'filter' => '',        'add_prefix' => 1),
    array('escape' => 0, 'filter' => 'raw',     'add_prefix' => 1),
    array('escape' => 0, 'filter' => 'escape',  'add_prefix' => 1),
    array('escape' => 1, 'filter' => '',        'add_prefix' => 0),
    array('escape' => 1, 'filter' => 'raw',     'add_prefix' => 0),
    array('escape' => 1, 'filter' => 'escape',  'add_prefix' => 0),
    array('escape' => 0, 'filter' => '',        'add_prefix' => 0),
    array('escape' => 0, 'filter' => 'raw',     'add_prefix' => 0),
    array('escape' => 0, 'filter' => 'escape',  'add_prefix' => 0),
);

$i = 0;
foreach ($data as $i_data) {
    echo 'TEST #', ++$i, ":\n";
    $auto_escape = $i_data['escape'];
    $filter = $i_data['filter'];
    $prefix = $i_data['add_prefix'] ? '$' : '';
    ini_set('blitz.auto_escape', $auto_escape);
    $T = new Blitz();
    $body = 'hey, {{ '.$prefix.'x ';
    if ($filter) {
        $body .= '| '.$filter.' ';
    }
    $body .= '}}';
    $T->load($body);
    echo "auto_escape = ".$auto_escape.", body = ".$body."\n";
    $T->display(array('x' => '<br>'));
    echo "\n\n";
    unset($T);
}


?>
--EXPECT--
this is &apos;a&apos; &quot;test&quot; &lt;a href=&quot;null&quot;&gt; &lt;&gt;@#$%^&amp;*()_=&apos;&quot; ��this is &apos;a&apos; &quot;test&quot; &lt;a href=&quot;null&quot;&gt; &lt;&gt;@#$%^&amp;*()_=&apos;&quot; ��
TEST #1:
auto_escape = 1, body = hey, {{ $x }}
hey, &lt;br&gt;

TEST #2:
auto_escape = 1, body = hey, {{ $x | raw }}
hey, <br>

TEST #3:
auto_escape = 1, body = hey, {{ $x | escape }}
hey, &lt;br&gt;

TEST #4:
auto_escape = 0, body = hey, {{ $x }}
hey, <br>

TEST #5:
auto_escape = 0, body = hey, {{ $x | raw }}
hey, <br>

TEST #6:
auto_escape = 0, body = hey, {{ $x | escape }}
hey, &lt;br&gt;

TEST #7:
auto_escape = 1, body = hey, {{ x }}
hey, &lt;br&gt;

TEST #8:
auto_escape = 1, body = hey, {{ x | raw }}
hey, <br>

TEST #9:
auto_escape = 1, body = hey, {{ x | escape }}
hey, &lt;br&gt;

TEST #10:
auto_escape = 0, body = hey, {{ x }}
hey, <br>

TEST #11:
auto_escape = 0, body = hey, {{ x | raw }}
hey, <br>

TEST #12:
auto_escape = 0, body = hey, {{ x | escape }}
hey, &lt;br&gt;


