complex fetch example {{ $title }}
{{ BEGIN root }}
this is root {{ $p1 }}
{{ BEGIN a }}
A-begin({{ $i }}) {{ BEGIN b }} B-begin; y={{$y}} B-end{{ END }} ; x={{$x}}, y={{$y}} A-end
{{ END }}
end of root {{ $p2 }}
{{ END }}
