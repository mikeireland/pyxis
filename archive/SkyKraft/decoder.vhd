--------------------------------------------------------------------------------
-- Company: <Name>
--
-- File: decoder.vhd
-- File history:
--      <Revision number>: <Date>: <Comments>
--      <Revision number>: <Date>: <Comments>
--      <Revision number>: <Date>: <Comments>
--
-- Description: 
--
-- decodes the NANEYE camera data from samples at 140MHz
--
-- Targeted device: <Family::SmartFusion2> <Die::M2S010> <Package::400 VF>
-- Author: <Name>
--
--------------------------------------------------------------------------------

library IEEE;

use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity decoder is
generic (
    g_WIDTH : integer := 30;	-- number of bits in the width of a buffer (can hold 3 x 10 bit pixels)
    --g_DEPTH : integer := 64
    );
port (
	i_Clk : IN  std_logic; -- clock
    --i_Start : IN std_logic; -- command to start sampling and filling buffers
    --i_Read_Buffer_index : IN unsigned(7 downto 0);
    i_Rst : IN  std_logic; -- reset
    i_Sample : IN  std_logic; -- RX signal to sample
	o_RecordingBits : OUT std_logic;
    o_data : OUT std_logic_vector(31 downto 0)  -- 32 bits of output data
    --o_buffer_index : OUT unsigned(31 downto 0)
);
end decoder;
architecture architecture_decoder of decoder is
   -- these commented out signals are from the old 64 cycling buffer system
    --type t_FIFO_DATA is array (0 to g_DEPTH-1) of std_logic_vector(g_WIDTH-1 downto 0);
    --signal r_Data : t_FIFO_DATA := (others => (others => '0'));
    --signal r_Bit_Index : integer range 0 to g_WIDTH-1 := 0;  -- 32 Bits Total
   -- signal r_Fill_Buffer_Index : integer range 0 to g_DEPTH := 0; -- keep track of which buffer we are filling
    --signal r_Read_Buffer_Index : integer range 0 to g_DEPTH := 0; -- keep track of which buffer should be read
    --signal r_Read_Buffer_Index_bits : std_logic_vector(7 downto 0) := "00000000";
	
	-- these signals are used to store the decoded bits
	signal bitIndex : integer range 0 to 11 := 0;	-- bit number within a pixel
	signal blockPosition : integer range 0 to 2 := 0;	-- which block out of the 3 pixels that can fit in each buffer
	signal FillingBuffer : std_logic_vector(29 downto 0) := (others => '0');	-- the buffer that is being filled
	signal OutputBuffer : std_logic_vector(29 downto 0) := (others => '0');		-- a full buffer of the previously decoded bits, to be read by the microcontroller over the APB bus
	signal SwitchBufferFlag : std_logic := '0';									-- this flag alternates whenever a new buffer is overwritten into the OutputBuffer
    
    signal r_Sample_data_R : std_logic := '0'; -- unregistered data directly sample, used to ensure metastability, I'm not sure if it is strictly needed (since we use each sample in XYZ) but it doesn't hurt to keep it
    
    signal Z : std_logic := '0';
    signal Y : std_logic := '0';
    signal X : std_logic := '0';
    signal ZeroCount : integer := 0;
    signal StartAdjustFrequency : std_logic := '0';
    signal AdjustFrequency : std_logic := '0';
    signal ClocksCount : unsigned(8 downto 0) := "000000000";
    signal TransitionCount : unsigned(12 downto 0) := "0000000000000";
    signal Phase : unsigned(8 downto 0) := "000000000";
    signal Freq : unsigned(8 downto 0) := "010100000";	-- 160 in decimal, from measurement
    signal Delta : unsigned(8 downto 0) := "000000000";
    
    signal PreBits : std_logic := '0';						-- flag that indicates when we are in Phase# 253a
    signal RecordingBits : std_logic := '0';
    signal BitsRecorded : integer range 0 to 756000 := 0; -- 12bits * (249+3)pixels * 250 rows
    constant c_NumFrameBits : unsigned(19 downto 0) := "10111000100100100000"; -- 756000 decimal

	constant c_HalfPeriod : unsigned(8 downto 0) := "100000000"; -- 256 decimal
	constant c_TotalSyncTransitions : unsigned(12 downto 0) := "1011101010110"; -- 5974 decimal -> 12bits * 2 edges * 249 pixels -2 becuase we miss the first two transitions due to the StartAdjustFrequency registering
	--constant c_TotalSyncTransitions : unsigned(12 downto 0) := "0001111101000"; -- 1000 decimal -> should just be some way into the syncronisation 0s, if we start sampling at this point, we would expect so decode pure zeros
																
begin
  -- Purpose: Double-register the incoming data.
  p_sample : process (i_Clk)
  begin
    if rising_edge(i_Clk) then
        r_Sample_data_R <= i_Sample;
	    Z <= Y;
	    Y <= X;
	    X <= r_Sample_data_R;
	    
    end if;
  end process p_sample;


    -- Purpose: perform decoding and store bits into OutputBuffer
p_fill_buffers : process (i_Clk,i_Rst)
begin
    if (i_Rst = '0') then
		-- reset bit buffers
        bitIndex <= 0;
        SwitchBufferFlag <= '0';
        OutputBuffer <= (others => '0');
        FillingBuffer <= (others => '0');
		
		-- reset signals
        ZeroCount <= 0;
		StartAdjustFrequency <= '0';
		AdjustFrequency <= '0';
		ClocksCount <= "000000000";
		TransitionCount <= "0000000000000";
		Phase <= "000000000";
		Freq <= "010100000";	-- 160 in decimal, from measurement, this is adjusted each frame
		Delta <= "000000000";
		
		PreBits <= '0';
		RecordingBits <= '0';
		BitsRecorded <= 0;
		
    elsif rising_edge(i_Clk) then
	
		if X = '1' then -- zero count is actually a 1 count becuase there is a string of ones after the string of 0s, when we are meant to configure the camera.
						-- we can change this to detecting the end of 0s and then timing the approx length of the configuration, and then we start syncing
			ZeroCount <= ZeroCount + 1;
		else
		-- X = '1'
			ZeroCount <= 0;
			if ZeroCount > 7 then
				StartAdjustFrequency <= '1';
			end if;
		end if;
		
		-- StartAdjustFrequency is a flag for the first clock cycle of the syncing, to reset all the required signals
		if StartAdjustFrequency = '1' then
			AdjustFrequency <= '1';
			ClocksCount <= "000000000";
			TransitionCount <= "0000000000000";
			StartAdjustFrequency <= '0';
		else
			ClocksCount <= ClocksCount + 1;
			if (X = '0' and Y = '1') or (X = '1' and Y = '0') then 
				TransitionCount <= TransitionCount + 1;
			end if;
		end if;
		
		if AdjustFrequency = '1' then
			if (X = '0') and (Y = '1') and (Z = '0') then
				Phase <= to_unsigned(128, Phase'length) + Freq;
			elsif (X = '1') and (Y = '0') and (Z = '1') then
				Phase <= to_unsigned(384, Phase'length) + Freq;
			else 
				Phase <= Phase + Freq;
			end if;
			if ClocksCount = 256 then
				Freq <= TransitionCount(8 downto 0);
				AdjustFrequency <= '0';
				Delta <= shift_right((c_HalfPeriod-TransitionCount(8 downto 0)),1); -- D = (256 - F)/2
			end if;
		else
			-- not AdjustFrequency
			if (Phase > c_HalfPeriod - Delta) and (Phase < c_HalfPeriod) and (Y = X) then
					Phase <= c_HalfPeriod + Freq;
			elsif (Phase > c_HalfPeriod) and (Phase < c_HalfPeriod + Delta) and (Y = Z) then
					Phase <= c_HalfPeriod + Freq;
			else
				Phase <= Phase + Freq;
			end if;
			
			if (TransitionCount = c_TotalSyncTransitions - 1) then -- we need to do it one clock cycle early to allow it to be ready to sample the first bit on the correct cycle.
				--PreBits <= '1';
				RecordingBits <= '1';	-- currently skipping the 12 PreBits so that we record all the data
			end if;
			
			if ( (RecordingBits = '1' or PreBits = '1') and ((Phase > Delta) and (Phase < c_HalfPeriod - Delta))) then
				-- increment bit counter
				BitsRecorded <= BitsRecorded + 1;
				
				if PreBits = '1' then
					if BitsRecorded = 12 then
						-- start of actual frame Phase# 1.1
						BitsRecorded <= 0;
						RecordingBits <= '1';
						PreBits <= '0';
					end if;
				end if;
					
				if RecordingBits = '1' then
				-- record a bit as the complement of Y
					if bitIndex = 0 then
						bitIndex <= 1;
					elsif (bitIndex > 0) and (bitIndex < 11) then
						if blockPosition = 0 then
							FillingBuffer(bitIndex -1) <= not Y;
						elsif blockPosition = 1 then
							FillingBuffer(9 + bitIndex) <= not Y;
						else
							FillingBuffer(19 + bitIndex) <= not Y;
						end if;
						bitIndex <= bitIndex + 1;
					else
						-- bitIndex = 11
						if blockPosition < 2 then
							blockPosition <= blockPosition + 1;
						else
							blockPosition <= 0;
							OutputBuffer <= FillingBuffer;
							SwitchBufferFlag <= not SwitchBufferFlag;
						end if;
						bitIndex <= 0;
					end if;
						
				end if;
							
			end if;
			
			if BitsRecorded = c_NumFrameBits then
				RecordingBits <= '0';
			end if;

		end if;
		
    end if;
    
end process p_fill_buffers;
      
    
    --o_data(0) <= AdjustFrequency;
	--o_data(9 downto 1) <= std_logic_vector(Freq);
	--o_data(18 downto 10) <= std_logic_vector(Delta);
	o_data(29 downto 0) <= OutputBuffer;
	o_data(31) <= SwitchBufferFlag;
    --o_buffer_index <= to_unsigned(r_Read_Buffer_Index, o_buffer_index'length);
	o_RecordingBits <= RecordingBits;

end architecture_decoder;