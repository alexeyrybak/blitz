--TEST--
testing arithmetics2
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
<table>
<tr>
{{ BEGIN data }}
{{ IF (_num > 1) && (_num - 1) % 3 == 0 }}<tr>{{ END }}
<td>{{ val }}</td>
{{ IF !_last && (_num - 1) % 3 == 2 }}</tr>{{ END }}
{{ END data }}
</tr>
</table>

BODY;

$T = new Blitz();
$T->load($body);

$T->display(
	array(
		'data' => array(
			array('val' => 0),
			array('val' => 1),
			array('val' => 2),
			array('val' => 3),
			array('val' => 4),
			array('val' => 5),
			array('val' => 6)
		)
	)
);
?>
--EXPECT--
<table>
<tr>

<td>0</td>


<td>1</td>


<td>2</td>
</tr>
<tr>
<td>3</td>


<td>4</td>


<td>5</td>
</tr>
<tr>
<td>6</td>

</tr>
</table>
