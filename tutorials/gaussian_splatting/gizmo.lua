-- A small world-orientation overlay, drawn after the splats in the same sequential view.
return function(hg)
    local vertices=hg.Vertices(hg.VertexLayoutPosFloatColorUInt8(),128)
    local colors={hg.Color(1,0,0),hg.Color(0,1,0),hg.Color(0,0.35,1)}
    local glyphs={{{-3,-4,3,4},{-3,4,3,-4}},{{-3,4,0,0},{3,4,0,0},{0,0,0,-4}},{{-3,4,3,4},{3,4,-3,-4},{-3,-4,3,-4}}}
    return function(view,program,state,width,height,right,up)
        vertices:Clear()
        local index=0
        local ox,oy=width-76,76
        local function line(x1,y1,x2,y2,color)
            vertices:Begin(index):SetPos(hg.Vec3(x1/width*2-1,y1/height*2-1,0)):SetColor0(color):End()
            vertices:Begin(index+1):SetPos(hg.Vec3(x2/width*2-1,y2/height*2-1,0)):SetColor0(color):End()
            index=index+2
        end
        for axis=1,3 do
            local dx,dy=right[axis]*42,up[axis]*42
            local length=math.sqrt(dx*dx+dy*dy)
            local x,y=ox+dx,oy+dy
            line(ox,oy,x,y,colors[axis])
            local nx,ny=length>1 and dx/length or .707,length>1 and dy/length or .707
            line(x,y,x-nx*6-ny*3,y-ny*6+nx*3,colors[axis])
            line(x,y,x-nx*6+ny*3,y-ny*6-nx*3,colors[axis])
            x,y=x+nx*10,y+ny*10
            for _,segment in ipairs(glyphs[axis]) do line(x+segment[1],y+segment[2],x+segment[3],y+segment[4],colors[axis]) end
        end
        hg.DrawLines(view,vertices,program,state)
    end
end
