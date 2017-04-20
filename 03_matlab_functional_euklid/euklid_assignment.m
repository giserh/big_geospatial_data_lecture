1;
% Implement the GGT algorithm of Euclid in its 
% recursive form  just as in the following
% imperative recursive implementation using
% functional programming in MATLAB 

function ret = rggt(a,b)
   disp(sprintf("%d -- %d", a,b))
   if(b == 0)
     ret = a;
   else
     ret = rggt(b, mod(a,b));
   endif
endfunction

%
% Therefore, use the following expressions from
% http://blogs.mathworks.com/loren/2013/02/07/introduction-to-functional-programming-with-anonymous-functions-part-3/
%


iif     = @(varargin) varargin{2*find([varargin{1:2:end}], 1, 'first')}();
recur   = @(f, varargin) f(f, varargin{:});
curly   = @(x, varargin) x{varargin{:}};
loop = @(x0, cont, fcn) ...                                     % Header
       recur(@(f, x) iif(~cont(x{:}), x, ...                    % Continue?
                         true,        @() f(f, fcn(x{:}))), ... %   Iterate
             x0);                                               % from x0.

% Add your code here
% The code below assumes a function
% fggt taking two arguments a and b and returning one value, just as rggt does for the recursive implementation.

	     
a = int8(rand()*100);
b = int8(rand()*100);

disp(sprintf("Theg gcd of %d and %d is %d. True gcd is %d.",a,b,fggt(a,b),rggt(a,b)))
