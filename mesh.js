
const VERTEX_STRIDE = 36;
const SPHERE_STRIDE = 48;

class Mesh {
    /** 
     * Creates a new mesh and loads it into video memory.
     * 
     * @param {WebGLRenderingContext} gl  
     * @param {number} program
     * @param {number[]} vertices
     * @param {number[]} indices
    */
    constructor( gl, program, vertices, indices, material) {
        this.verts = create_and_load_vertex_buffer( gl, vertices, gl.STATIC_DRAW );
        this.indis = create_and_load_elements_buffer( gl, indices, gl.STATIC_DRAW );
        this.material = material;

        this.n_verts = vertices.length;
        this.n_indis = indices.length;
        this.program = program;
    }

    static from_heightmap(gl, program, map, min, max, material) {
        let rows = map.length;
        let cols = map[0].length;

        const MIN_HEIGHT_COLOR = 0.2;

        let off_x = cols /2;
        let off_z = rows /2;
        
        let verts = [];
        let indis = [];

        function color(height) {
            let normed_height = (height - min) / (max - min);
            return MIN_HEIGHT_COLOR + normed_height;
        }
        let indi_start = 0;
        for(let row = 1; row < rows; row++) {
            for(let col = 1; col < cols; col++) { 
                let pos_tl = map[row-1][col-1];
                let pos_tr = map[row-1][col];
                let pos_bl = map[row][col-1];
                let pos_br = map[row][col];

                let v_tl = new Vec4(-1, pos_tl, -1, 1);
                let v_tr = new Vec4(0, pos_tr, -1, 1);
                let v_bl = new Vec4(-1, pos_bl, 0, 1);
                let v_br = new Vec4(0, pos_br, 0, 1);

                let normal_t1 = Vec4.normal_of_triangle(v_tl, v_tr, v_bl);
                let t1_mag = normal_t1.length();
                normal_t1 = new Vec4((normal_t1.x)/t1_mag, (normal_t1.y)/t1_mag,(normal_t1.z)/t1_mag);
                let normal_t2 = Vec4.normal_of_triangle(v_br, v_bl, v_tr);
                let t2_mag = normal_t2.length();
                normal_t2 = new Vec4((normal_t2.x)/t2_mag, (normal_t2.y)/t2_mag,(normal_t2.z)/t2_mag);

                v_tl.x += col - off_x;
                v_tl.z += row - off_z;

                v_tr.x += col - off_x;
                v_tr.z += row - off_z;

                v_bl.x += col - off_x;
                v_bl.z += row - off_z;

                v_br.x += col - off_x;
                v_br.z += row - off_z;

                function push_vert(list, vert, u, v, norm){
                    list.push(vert.x);
                    list.push(vert.y);
                    list.push(vert.z);
                    list.push(1,1,1,1);
                    list.push(u);
                    list.push(v);
                    list.push(norm.x);
                    list.push(norm.y);
                    list.push(norm.z);
                }

                push_vert(verts, v_tl, 0,1, normal_t1);
                push_vert(verts, v_tr, 1,1, normal_t1);
                push_vert(verts, v_bl, 0,0, normal_t1);

                push_vert(verts, v_br, 1,0, normal_t2);
                push_vert(verts, v_bl, 0,0, normal_t2);
                push_vert(verts, v_tr, 1,1, normal_t2);

                for(let i = 0; i < 6; i++) {
                    indis.push(indi_start++);
                }
            }
        }

        return new Mesh(gl, program, verts, indis, material, true);

    }

    static inside_box(gl, program, width, height, depth, material){
        let hwidth = width / 2.0;
        let hheight = height / 2.0;
        let hdepth = depth / 2.0;

            //let normals = new Vec4(x, y, z, 0.0).norm();
            //verts.push(normals.x, normals.y, normals.z);

        let one = new Vec4(hwidth, -hheight, -hdepth, 0.0).norm();
        let two = new Vec4(-hwidth, -hheight, -hdepth, 0.0).norm();
        let three = new Vec4(-hwidth, hheight, -hdepth, 0.0).norm();
        let four = new Vec4(hwidth, hheight, -hdepth, 0.0).norm();
        let five = new Vec4(hwidth, -hheight, hdepth, 0.0).norm();
        let six = new Vec4(-hwidth, -hheight, hdepth, 0.0).norm();
        let seven = new Vec4(-hwidth, hheight, hdepth, 0.0).norm();
        let eight = new Vec4(hwidth, hheight, hdepth, 0.0).norm();

        let verts = [
            //x, y, z, r, g, b, a, u, v, normals
            hwidth, -hheight, -hdepth, 1.0, 0.0, 0.0, 1.0,    0.25  , 0.5  , -one.x, -one.y, -one.z,//0
            hwidth, -hheight, -hdepth, 1.0, 0.0, 0.0, 1.0,    0.25 , 0.5 , -one.x, -one.y, -one.z,//1
            hwidth, -hheight, -hdepth, 1.0, 0.0, 0.0, 1.0,    0.5 , 0.75 , -one.x,-one.y,-one.z, //2

            -hwidth, -hheight, -hdepth, 0.0, 1.0, 0.0, 1.0,   0 , 0.5 , -two.x,-two.y,-two.z,//3
            -hwidth, -hheight, -hdepth, 0.0, 1.0, 0.0, 1.0,   1 , 0.5 , -two.x, -two.y, -two.z,//4
            -hwidth, -hheight, -hdepth, 0.0, 1.0, 0.0, 1.0,   0.75 , 0.75 , -two.x,-two.y,-two.z,//5

            -hwidth, hheight, -hdepth, 0.0, 0.0, 1.0, 1.0,   0 , 0.25 , -three.x, -three.y, -three.z,//6 
            -hwidth, hheight, -hdepth, 0.0, 0.0, 1.0, 1.0,   0.75 , 0 ,-three.x, -three.y, -three.z,//7
            -hwidth, hheight, -hdepth, 0.0, 0.0, 1.0, 1.0,   1 , 0.25 ,-three.x, -three.y, -three.z,//8

            hwidth, hheight, -hdepth, 1.0, 1.0, 0.0, 1.0,    0.25 , 0.25 , -four.x, -four.y, -four.z,//9
            hwidth, hheight, -hdepth, 1.0, 1.0, 0.0, 1.0,    0.25 , 0.25 ,-four.x, -four.y, -four.z,//10
            hwidth, hheight, -hdepth, 1.0, 1.0, 0.0, 1.0,    0.5 , 0 , -four.x, -four.y, -four.z,//11  

            hwidth, -hheight, hdepth, 1.0, 0.0, 1.0, 1.0,    0.5 , 0.5 , -five.x, -five.y, -five.z,//12
            hwidth, -hheight, hdepth, 1.0, 0.0, 1.0, 1.0,    0.5 , 0.5 , -five.x, -five.y, -five.z,//13
            hwidth, -hheight, hdepth, 1.0, 0.0, 1.0, 1.0,    0.5 , 0.5 ,-five.x, -five.y, -five.z,//14

            -hwidth, -hheight, hdepth, 0.0, 1.0, 1.0, 1.0,   0.75 , 0.5 , -six.x, -six.y, -six.z, //15
            -hwidth, -hheight, hdepth, 0.0, 1.0, 1.0, 1.0,   0.75 , 0.5 ,-six.x, -six.y, -six.z, //16
            -hwidth, -hheight, hdepth, 0.0, 1.0, 1.0, 1.0,   0.75 , 0.5 ,-six.x, -six.y, -six.z, //17

            -hwidth, hheight, hdepth, 0.5, 0.5, 1.0, 1.0,    0.75 , 0.25 ,-seven.x, -seven.y, -seven.z,//18
            -hwidth, hheight, hdepth, 0.5, 0.5, 1.0, 1.0,    0.75 , 0.25 ,-seven.x, -seven.y, -seven.z,//19
            -hwidth, hheight, hdepth, 0.5, 0.5, 1.0, 1.0,    0.75 , 0.25 ,-seven.x, -seven.y, -seven.z, //20

            hwidth, hheight, hdepth, 1.0, 1.0, 0.5, 1.0,     0.5 , 0.25 ,-eight.x, -eight.y, -eight.z,//21
            hwidth, hheight, hdepth, 1.0, 1.0, 0.5, 1.0,     0.5 , 0.25 , -eight.x, -eight.y, -eight.z,//22
            hwidth, hheight, hdepth, 1.0, 1.0, 0.5, 1.0,     0.5 , 0.25 , -eight.x, -eight.y, -eight.z,//23
        ];

        let indis = [
            // clockwise winding
            /*
            0, 1, 2, 2, 3, 0, 
            4, 0, 3, 3, 7, 4, 
            5, 4, 7, 7, 6, 5, 
            1, 5, 6, 6, 2, 1,
            3, 2, 6, 6, 7, 3,
            4, 5, 1, 1, 0, 4,
            */

            // counter-clockwise winding
            0, 9, 3, 3, 9, 6, //front
            12, 21, 1, 1, 21, 10, //right
            15, 18, 13, 13, 18, 22, //back
            4, 8, 16, 16, 8, 20, //left
            19, 7, 23, 23, 7, 11, //top
            5, 17, 2, 2, 17, 14 //bottom
        ];
        gl.bindTexture(gl.TEXTURE_2D, material.texture);

        return new Mesh( gl, program, verts, indis, material);
    }
    /**
     * Create a box mesh with the given dimensions and colors.
     * @param {WebGLRenderingContext} gl 
     * @param {number} width 
     * @param {number} height 
     * @param {number} depth 
     */

    static box( gl, program, width, height, depth, material) {
        let hwidth = width / 2.0;
        let hheight = height / 2.0;
        let hdepth = depth / 2.0;

            //let normals = new Vec4(x, y, z, 0.0).norm();
            //verts.push(normals.x, normals.y, normals.z);

        let one = new Vec4(hwidth, -hheight, -hdepth, 0.0).norm();
        let two = new Vec4(-hwidth, -hheight, -hdepth, 0.0).norm();
        let three = new Vec4(-hwidth, hheight, -hdepth, 0.0).norm();
        let four = new Vec4(hwidth, hheight, -hdepth, 0.0).norm();
        let five = new Vec4(hwidth, -hheight, hdepth, 0.0).norm();
        let six = new Vec4(-hwidth, -hheight, hdepth, 0.0).norm();
        let seven = new Vec4(-hwidth, hheight, hdepth, 0.0).norm();
        let eight = new Vec4(hwidth, hheight, hdepth, 0.0).norm();

        let verts = [
            //x, y, z, r, g, b, a, u, v, normals
            hwidth, -hheight, -hdepth, 1.0, 0.0, 0.0, 1.0,    0.25  , 0.5  , one.x, one.y, one.z,//0
            hwidth, -hheight, -hdepth, 1.0, 0.0, 0.0, 1.0,    0.25 , 0.5 , one.x, one.y, one.z,//1
            hwidth, -hheight, -hdepth, 1.0, 0.0, 0.0, 1.0,    0.5 , 0.75 , one.x,one.y,one.z, //2

            -hwidth, -hheight, -hdepth, 0.0, 1.0, 0.0, 1.0,   0 , 0.5 , two.x,two.y,two.z,//3
            -hwidth, -hheight, -hdepth, 0.0, 1.0, 0.0, 1.0,   1 , 0.5 , two.x, two.y, two.z,//4
            -hwidth, -hheight, -hdepth, 0.0, 1.0, 0.0, 1.0,   0.75 , 0.75 , two.x,two.y,two.z,//5

            -hwidth, hheight, -hdepth, 0.0, 0.0, 1.0, 1.0,   0 , 0.25 , three.x, three.y, three.z,//6 
            -hwidth, hheight, -hdepth, 0.0, 0.0, 1.0, 1.0,   0.75 , 0 ,three.x, three.y, three.z,//7
            -hwidth, hheight, -hdepth, 0.0, 0.0, 1.0, 1.0,   1 , 0.25 ,three.x, three.y, three.z,//8

            hwidth, hheight, -hdepth, 1.0, 1.0, 0.0, 1.0,    0.25 , 0.25 , four.x, four.y, four.z,//9
            hwidth, hheight, -hdepth, 1.0, 1.0, 0.0, 1.0,    0.25 , 0.25 ,four.x, four.y, four.z,//10
            hwidth, hheight, -hdepth, 1.0, 1.0, 0.0, 1.0,    0.5 , 0 , four.x, four.y, four.z,//11  

            hwidth, -hheight, hdepth, 1.0, 0.0, 1.0, 1.0,    0.5 , 0.5 , five.x, five.y, five.z,//12
            hwidth, -hheight, hdepth, 1.0, 0.0, 1.0, 1.0,    0.5 , 0.5 , five.x, five.y, five.z,//13
            hwidth, -hheight, hdepth, 1.0, 0.0, 1.0, 1.0,    0.5 , 0.5 ,five.x, five.y, five.z,//14

            -hwidth, -hheight, hdepth, 0.0, 1.0, 1.0, 1.0,   0.75 , 0.5 , six.x, six.y, six.z, //15
            -hwidth, -hheight, hdepth, 0.0, 1.0, 1.0, 1.0,   0.75 , 0.5 ,six.x, six.y, six.z, //16
            -hwidth, -hheight, hdepth, 0.0, 1.0, 1.0, 1.0,   0.75 , 0.5 ,six.x, six.y, six.z, //17

            -hwidth, hheight, hdepth, 0.5, 0.5, 1.0, 1.0,    0.75 , 0.25 ,seven.x, seven.y, seven.z,//18
            -hwidth, hheight, hdepth, 0.5, 0.5, 1.0, 1.0,    0.75 , 0.25 ,seven.x, seven.y, seven.z,//19
            -hwidth, hheight, hdepth, 0.5, 0.5, 1.0, 1.0,    0.75 , 0.25 ,seven.x, seven.y, seven.z, //20

            hwidth, hheight, hdepth, 1.0, 1.0, 0.5, 1.0,     0.5 , 0.25 ,eight.x, eight.y, eight.z,//21
            hwidth, hheight, hdepth, 1.0, 1.0, 0.5, 1.0,     0.5 , 0.25 , eight.x, eight.y, eight.z,//22
            hwidth, hheight, hdepth, 1.0, 1.0, 0.5, 1.0,     0.5 , 0.25 , eight.x, eight.y, eight.z,//23
        ];

        let indis = [
            // clockwise winding
            /*
            0, 1, 2, 2, 3, 0, 
            4, 0, 3, 3, 7, 4, 
            5, 4, 7, 7, 6, 5, 
            1, 5, 6, 6, 2, 1,
            3, 2, 6, 6, 7, 3,
            4, 5, 1, 1, 0, 4,
            */

            // counter-clockwise winding
            0, 9, 3, 3, 9, 6, //front
            12, 21, 1, 1, 21, 10, //right
            15, 18, 13, 13, 18, 22, //back
            4, 8, 16, 16, 8, 20, //left
            19, 7, 23, 23, 7, 11, //top
            5, 17, 2, 2, 17, 14 //bottom
        ];
        gl.bindTexture(gl.TEXTURE_2D, material.texture);

        return new Mesh( gl, program, verts, indis, material);
    }

    static make_uv_sphere(gl, program, subdivs, material, invert_normals){
        let verts = [];
        let indis = [];
        let n = 1;
        if(invert_normals) n = -1;
        const TAU = Math.PI * 2;
        //for each layer
        for (let layer = 0; layer <=subdivs; layer++){
            let y_turns = layer / subdivs / 2;
            let rs = Math.sin(2 * Math.PI * y_turns);
            let y = Math.cos(y_turns * TAU) / 2;
            let vertices = subdivs+1;
            
            for(let subdiv = 0; subdiv <= subdivs; subdiv++){
                let turns = subdiv/subdivs;
                let rads = turns * TAU;
                let x = Math.cos(rads)/2 * rs;
                let z = Math.sin(rads)/2 * rs;
                let u = subdiv/subdivs;
                let v = layer/subdivs;
                verts.push(x,y,z); //add x, y, z, r, g, b, a, u, v, norm x, norm y, norm z
                verts.push(1.0,1.0,1.0,1.0);
                verts.push(u,v);
                let normals = new Vec4(x, y, z, 0.0).norm();
                verts.push( n * normals.x, n * normals.y, n * normals.z);
            }

            if (layer > 0){
                for (let i = 0; i < vertices; i++){
                    if (i == vertices - 1){ //for the last quad
                        let bottomLeft = (layer*vertices) + i; 
                        let bottomRight = (layer*vertices);
                        let topRight =  (layer-1)*vertices + i + 1;
                        let topLeft = ((layer-1)*vertices) + i;
                        indis.push(bottomLeft, bottomRight, topRight, topRight, topLeft, bottomLeft);
                    }else{
                        let bottomLeft = layer * vertices + i;
                        let bottomRight = layer * vertices + i + 1;
                        let topRight = (layer-1) * vertices + i + 1;
                        let topLeft = (layer-1) * vertices + i;
                        indis.push(bottomLeft, bottomRight, topRight, topRight, topLeft, bottomLeft);
                    }
                    
                }
            }
        }
        gl.bindTexture(gl.TEXTURE_2D, material.texture);
        return new Mesh(gl, program, verts, indis, material);
    }

   /**
     * Render the mesh. Does NOT preserve array/index buffer or program bindings! 
     * 
     * @param {WebGLRenderingContext} gl 
     */
   render( gl ) {
        gl.cullFace( gl.BACK );
        // gl.enable( gl.CULL_FACE );
        gl.frontFace(gl.CW);
        
        gl.activeTexture(gl.TEXTURE0);
        gl.bindTexture(gl.TEXTURE_2D, this.material.texture);
        set_uniform_scalar(gl, this.program, 'mat_ambient', this.material.ambient);
        set_uniform_scalar(gl, this.program, 'mat_diffuse', this.material.diffuse);
        set_uniform_scalar(gl, this.program, 'mat_specular', this.material.specular);
        set_uniform_scalar(gl, this.program, 'mat_shininess', this.material.shininess);
        
        gl.useProgram( this.program );
        gl.bindBuffer( gl.ARRAY_BUFFER, this.verts );
        gl.bindBuffer( gl.ELEMENT_ARRAY_BUFFER, this.indis );

        set_vertex_attrib_to_buffer( 
            gl, this.program, 
            "coordinates", 
            this.verts, 3, 
            gl.FLOAT, false, SPHERE_STRIDE, 0 
        );

        set_vertex_attrib_to_buffer( 
            gl, this.program, 
            "color", 
            this.verts, 4, 
            gl.FLOAT, false, SPHERE_STRIDE, 12
        );

        set_vertex_attrib_to_buffer(
            gl, this.program,
            "uv",
            this.verts, 2,
            gl.FLOAT, false, SPHERE_STRIDE, 28
        );

        set_vertex_attrib_to_buffer(
            gl, this.program,
            "normal",
            this.verts, 3,
            gl.FLOAT, false, SPHERE_STRIDE, 36
        );
        gl.drawElements( gl.TRIANGLES, this.n_indis, gl.UNSIGNED_SHORT, 0 );
    }

/*
    /**
     * Render the mesh. Does NOT preserve array/index buffer or program bindings! 
     * 
     * @param {WebGLRenderingContext} gl 
    render( gl ) {
        gl.cullFace( gl.BACK );
        gl.enable( gl.CULL_FACE );
        
        gl.useProgram( this.program );
        gl.bindBuffer( gl.ARRAY_BUFFER, this.verts );
        gl.bindBuffer( gl.ELEMENT_ARRAY_BUFFER, this.indis );

        set_vertex_attrib_to_buffer( 
            gl, this.program, 
            "coordinates", 
            this.verts, 3, 
            gl.FLOAT, false, VERTEX_STRIDE, 0 
        );

        set_vertex_attrib_to_buffer( 
            gl, this.program, 
            "color", 
            this.verts, 4, 
            gl.FLOAT, false, VERTEX_STRIDE, 12
        );

        set_vertex_attrib_to_buffer(
            gl, this.program,
            "uv",
            this.verts, 2,
            gl.FLOAT, false, VERTEX_STRIDE, 28
        );

        gl.drawElements( gl.TRIANGLES, this.n_indis, gl.UNSIGNED_SHORT, 0 );
    }
    */
    /**
     * Parse the given text as the body of an obj file.
     * @param {WebGLRenderingContext} gl
     * @param {WebGLProgram} program
     * @param {string} text
     */
    static from_obj_text( gl, program, text, texture_url) {
        let lines = text.split( /\r?\n/ );
        let tex = gl.createTexture();
        let tex_img = new Image();
        tex_img.src = texture_url;
        tex_img.onload = on_load_metal;
        function on_load_metal(){
            gl.bindTexture(gl.TEXTURE_2D, tex);
            gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, tex_img);
            gl.generateMipmap(gl.TEXTURE_2D);
        };
        let material = new Material(0.3,0.1,0.1,1, tex);
        let verts = [];
        let vertices = [];
        let uv = [];
        let norms = [];
        let indis = [];
        let i = 0
        for( let line of lines ) {
            let trimmed = line.trim();
            let parts = trimmed.split( /(\s+)/ );
            i++;

            if( 
                parts === null || parts.length < 2 || parts[0] == 'mtllib' ||
                parts[0] == '#' || parts[0] === '' || parts[0] == 'g' || parts[0] =='s' || parts[0] == 'usemtl' || parts[0]=='o') 
            { 
                continue; 
            }
            else if(parts[0] == 'vt') {
                // console.log("uv coord" + parts[2] + parts[4]);
                uv.push(parseFloat(parts[2]), parseFloat(parts[4]));
            }
            else if(parts[0]=='vn') {
                // vertex normal!
                norms.push(parseFloat(parts[2]), parseFloat(parts[4]), parseFloat(parts[6]));
            }
            else if( parts[0] == 'v') {
                
                vertices.push( parseFloat( parts[2] ) );
                vertices.push( parseFloat( parts[4] ) );
                vertices.push( parseFloat( parts[6] ) );
                
                // color data
                vertices.push( 1.0, 1.0, 1.0, 1.0 );
            }
            else if( parts[0] == 'f' ) {
                // console.log(line);
                let indices = [];
                for (let i = 1; i < parts.length; i++) {
                    let number = parseFloat(parts[i].split('/')[0])-1;
                    if(! isNaN(parseFloat(number))) {
                        indices.push(number);
                    }
                }
                // console.log(indices);
                indis.push(...indices);
            }
            else {
                console.log( parts) ;
                throw new Error( 'unsupported obj command: ', parts[0], parts, i);
            }
        }
        let vCount = 0;
        let uCount = 0;
        let normCount = 0;
        console.log(vertices.length/7);
        for (let i = 0; i < (vertices.length/7); i ++) {
            for (let j = 0; j < 7; j++){
                verts.push(vertices[vCount]);
                vCount++;
            }
            for (let j = 0; j < 2; j++){
                verts.push(uv[uCount]);
                uCount++;
            }
            for (let j = 0; j < 3; j ++){
                verts.push(norms[normCount]);
                normCount++;
            }
        }
        
        return new Mesh( gl, program, verts, indis, material );
    }

    /**
     * Asynchronously load the obj file as a mesh.
     * @param {WebGLRenderingContext} gl
     * @param {string} file_name 
     * @param {WebGLProgram} program
     * @param {function} f the function to call and give mesh to when finished.
     */
    static from_obj_file( gl, file_name, program, texture_url, f ) {
        let request = new XMLHttpRequest();

        request.onload = () => { 
            let loaded_mesh = Mesh.from_obj_text(gl, program, request.responseText, texture_url);
            console.log(loaded_mesh);
            f(loaded_mesh);
        }
        request.open( 'GET', file_name ); // initialize request. 
        request.send();                   // execute request
    }

    static empty_mesh(gl) {
        return new Mesh(gl,0,0,0,0);
    }
}