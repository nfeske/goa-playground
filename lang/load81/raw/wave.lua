function setup()
   apos=0
   bpos=0
   cpos=0
   -- This function is called only once at startup.
end

function draw()
   -- This function is called at every frame refresh.
    background(0,0,0)
    for x=0,WIDTH,20 do
        a=math.sin((3.14*x + apos)/(WIDTH/2))
        b=math.sin((2.8*x  + bpos)/(WIDTH/3))
        c=math.sin((0.77*x + cpos)/(WIDTH/5))

        y=100*a + 100*b + 100*c
        fill(128 + c*128,60+x/2,250-x,0.3)
        ellipse(x,y + HEIGHT/2,20*a*b+30,20*c*a+30)
    end
    bpos = bpos + 15.1
    apos = apos - 9.3
    cpos = cpos + 15.12
end
