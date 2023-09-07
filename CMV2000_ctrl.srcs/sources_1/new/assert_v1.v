module assert_v1 #(
    parameter COUNT_WIDTH = 8//计数器宽度
    )  (
    input  sclk,//SPI时钟
    input  ss_raw,//SPI片选

    output ss_assert_v1//输出的SPI片选
    );

    reg [COUNT_WIDTH - 1 : 0] count = 0;
    
    //输出片选的赋值
    assign ss_assert_v1 = ((count > 5) & (count < 11)) ? 1'b1 : (~ss_raw);

    //控制输出片选的计数器
    always @(posedge sclk) begin
    
        if(count < 15)
            count <= count + 1;

        else 
            count <= 0;
        
    end

endmodule
