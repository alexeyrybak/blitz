complex list example
{{ BEGIN list; }}list #{{ $list_num }}
{{ BEGIN list_empty; }} this list is empty
{{ END }}{{ BEGIN list_item; }} row #{{ $i_row; }}
{{ END }}
{{ END }}
