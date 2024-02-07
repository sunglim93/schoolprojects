class Node{
    constructor(data){
        this.position = new Vec4(0,0,0,1);
        this.scale = new Vec4(1,1,1,1);
        this.roll = 0;
        this.pitch = 0;
        this.yaw = 0;
        this.data = data;
        this.children = [];
    }

    add_child() {
        let child = new Node;
        this.children.push(child);
        return child;
    }

    get_matrix(){
        let matrix = new Mat4();
        matrix = matrix.mul(Mat4.translation(this.position.x,this.position.y, this.position.z));
        matrix = matrix.mul(Mat4.rotation_xz(this.yaw));
        matrix = matrix.mul(Mat4.rotation_yz(this.pitch));
        matrix = matrix.mul(Mat4.rotation_xy(this.roll));
        matrix = matrix.mul(Mat4.scale(this.scale.x, this.scale.y, this.scale.z));
        return matrix;
    }

    get_pitch() {
        return this.pitch;
    }
}