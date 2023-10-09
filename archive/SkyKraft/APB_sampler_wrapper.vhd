--------------------------------------------------------------------------------
-- Company: <Name>
--
-- File: APB_sampler_wrapper.vhd
-- File history:
--      <Revision number>: <Date>: <Comments>
--      <Revision number>: <Date>: <Comments>
--      <Revision number>: <Date>: <Comments>
--
-- Description: 
--
-- <Description here>
--
-- Targeted device: <Family::SmartFusion2> <Die::M2S010> <Package::400 VF>
-- Author: <Name>
--
--------------------------------------------------------------------------------

library IEEE;

use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity APB_sampler_wrapper is
    port(
        PCLK            : in  std_logic;                        -- APB clock
        PRESETn         : in  std_logic;                        -- APB reset
        PENABLE         : in  std_logic;                        -- APB enable
        PSEL            : in  std_logic;                        -- APB periph select
        PADDR           : in  std_logic_vector(4 downto 0);     -- APB address bus
        PWRITE          : in  std_logic;                        -- APB write
        PWDATA          : in  std_logic_vector(31 downto 0);    -- APB write data
        PRDATA          : out std_logic_vector(31 downto 0);    -- APB read data
        PREADY          : out std_logic;                        -- APB ready signal data
        PSLVERR         : out std_logic;                        -- APB error signal
        SAMPLE          : in std_logic                         -- naneye data input
        --FAST_CLK        : in std_logic                          -- fast clock for sampling
        --INT          : out std_logic                         --  interrupt     
    );
end APB_sampler_wrapper;
architecture architecture_APB_sampler_wrapper of APB_sampler_wrapper is
component decoder is
  port(
    i_Clk : IN  std_logic; -- clock
    --i_Start : IN std_logic; -- command to start sampling and filling buffers
    --i_Read_Buffer_index : IN unsigned(7 downto 0);
    i_Rst : IN  std_logic; -- reset
    i_Sample : IN  std_logic; -- RX signal to sample
	o_RecordingBits : OUT std_logic;
    o_data : OUT std_logic_vector(31 downto 0)
    );
end component;
   -- signal, component etc. declarations
    constant BUFFERCONTENTA     : std_logic_vector(4 downto 0) := "00000";
    --constant BUFFERNUMBERA      : std_logic_vector(4 downto 0) := "00100";
    constant BUFFERCONTROLA       : std_logic_vector(4 downto 0) := "01100";
	constant RECORDINGBITSA     : std_logic_vector(4 downto 0) := "11100";
    constant TESTA              : std_logic_vector(4 downto 0) := "01000";
    signal test_val : std_logic_vector(31 downto 0) := "10011001100110011001100110011001";
    
	signal r_buffer : std_logic_vector(31 downto 0) := (others => '0'); -- buffer to hold output from decoder
	--signal r_buffer_index : unsigned(7 downto 0); -- which buffer are we currently reading
	signal r_RecordingBits : std_logic := '0';
    
    --signal r_start_cmd                  : std_logic := '0';
    signal r_reset                 : std_logic := '1';  -- active low
    --signal r_buffer_next_cmd            : std_logic := '0';
    
    signal DataOut          : std_logic_vector(31 downto 0);
    signal DataOut_int      : std_logic_vector(31 downto 0);

begin
PREADY  <= '1'; 
PSLVERR <= '0'; 

p_reg_seq : process (PRESETn, PCLK)
    begin
        if (PRESETn = '0') then
            r_reset <= '0';
        elsif (PCLK'event and PCLK = '1') then
            if (PWRITE = '1' and PSEL = '1' and PENABLE = '1' and PADDR = BUFFERCONTROLA) then
                r_reset <= PWDATA (0);  -- active low reset
            --elsif (PWRITE = '1' and PSEL = '1' and PENABLE = '1' and PADDR = BUFFERNUMBERA) then
              --  r_buffer_index        <= unsigned(PWDATA(7 downto 0));
			elsif (PWRITE = '1' and PSEL = '1' and PENABLE = '1' and PADDR = TESTA) then
                test_val <= PWDATA(31 downto 0);
            end if;
        end if;
    end process p_reg_seq;

my_sampler: decoder
    port map(i_Clk    => PCLK,
       i_Sample       => SAMPLE,
       i_Rst          => r_reset,
       --i_Start         => r_start_cmd,
       --i_Read_Buffer_index   => r_buffer_index,
	   o_RecordingBits  => r_RecordingBits,
       o_data           => r_buffer);

-- prepare data for reads from the slave.
    p_data_out : process (PWRITE, PSEL, PADDR, r_buffer, r_RecordingBits, test_val)
    begin
        DataOut <= (others => '0'); -- Drive zeros by default
        if (PWRITE = '0' and PSEL = '1') then
            case PADDR is
                when BUFFERCONTENTA =>
                    DataOut(31 downto 0) <= r_buffer;

                --when BUFFERNUMBERA =>
                --    DataOut(7 downto 0) <= std_logic_vector(r_buffer_index);
                
                when TESTA =>
                    DataOut <= test_val;
					
				when RECORDINGBITSA =>
                    DataOut(0) <= r_RecordingBits;
                    
                when others =>
                    DataOut <= (others => '0');
            end case;
        else
            DataOut <= (others => '0');
        end if;
    end process p_data_out;
    
        -- Generate PRDATA on falling edge
    p_PRDATA : process (PRESETn, PCLK)
    begin
        if (PRESETn = '0') then
            DataOut_int <= (others => '0');
        elsif rising_edge(PCLK) then
            if (PWRITE = '0' and PSEL = '1') then
                DataOut_int <= DataOut;
            end if;
        end if;
    end process p_PRDATA;

PRDATA <= DataOut_int;

end architecture_APB_sampler_wrapper;