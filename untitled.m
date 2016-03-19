m = rsdata(:,12);

for i = 1:size(m,1)
    
    if isempty(cell2mat(m(i)))
        epsc(i) = NaN;
    else
        epsc(i) = round(cell2mat(m(i)));
    end
end
