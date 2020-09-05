local feature_map = {}
--local feature_map = xxxx
local face_num = 1
--local face_num = xxx

local skybox_feature_name = "NoSkybox"
local quaternionMultiply = function(first, second)
	local result = {}
	result[1] = first[1]*second[1]-first[2]*second[2]-first[3]*second[3]-first[4]*second[4]
	result[2] = first[1]*second[2]+first[2]*second[1]-first[3]*second[4]+first[4]*second[3]
	result[3] = first[1]*second[3]+first[2]*second[4]+first[3]*second[1]-first[4]*second[2]
	result[4] = first[1]*second[4]-first[2]*second[3]+first[3]*second[2]+first[4]*second[1]
	return result
end
--local skybox_feature_name = xxx

-- Meta class, implement a simple LRU cache
LRUCache = {slots = {},capacity = 0}
--construnctor
--capacity, the capacity of LRU cache
function LRUCache:new(o,capacity)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    self.capacity = capacity or 1
    self.slots = {}
    for i=1,capacity do
        self.slots[i]={time_stamp=0,value=0}
    end
    return o
end
--getKey
function LRUCache:getKey(value)
	local find = false
	local find_key = 0
	local override_key = 0
	local max_time_stamp = 0
	for i=1,self.capacity do
		self.slots[i].time_stamp = self.slots[i].time_stamp+1
		if(self.slots[i].value == value) then
			find = true
			find_key = i
			self.slots[i].time_stamp = 0
		end
		if(self.slots[i].time_stamp > max_time_stamp) then
			max_time_stamp = self.slots[i].time_stamp
			override_key = i
		end
	end
	if(not find) then
		find_key = override_key
		self.slots[find_key].time_stamp = 0
		self.slots[find_key].value = value
	end
    return find_key-1
end
local my_cache = LRUCache:new(nil,face_num)

    
--play Animation
local playAnimation = function(scene, entity_id, animType, animName, isLoop, isStop)
	if (isStop) then
		scene:stop(animType, entity_id, animName)
		return
	end	
	local isPlaying = scene:isAnimPlaying(animType, entity_id, animName)
	if(not isPlaying) then
		scene:play(animType, entity_id, animName, isLoop)
	end
end

--event handlers
--input faceindex
--input action
--output what animation should be triggered
EventHandles =
{
	handleFaceActionEvent = function (this, faceindex, action)
		local actual_face_index = my_cache:getKey(faceindex)
		if feature_map[actual_face_index] == nil then 
			return false
		end
		if feature_map[actual_face_index][action] == nil then
			return false
		end
		-- iterate the table
		for key, value in pairs(feature_map[actual_face_index][action]) do
			local feature = this:getFeature(value['feature_name'])
			local type_str = value['type_str']
			if (type_str == '3DSticker') then
				feature = EffectSdk.castSticker3DV3Feature(feature)
			elseif (type_str == 'AR') then
				feature = EffectSdk.castARV3Feature(feature)
			end
			if (feature) then
				local scene = feature:getLuaScene()
				playAnimation(scene, value['id'], value['anim_type'], value['anim_name'],value['is_loop'],value['trigger_stop'])
			end
		end
		return true				
	end
	,
    handleDeviceOrientationEvent = function ( this, orient_0, orient_1, orient_2, orient_3, isFront )
        local feature3d = this:getFeature(skybox_feature_name)
        feature3d = EffectSdk.castSticker3DV3Feature(feature3d)
        if( not feature3d ) then 
            return false
        end
        local scene = feature3d:getLuaScene()
        if( not scene ) then
            return false
        end
        
        local base_quaternion = {1,0,0,0}
		-- if( isFront ) then
		--	-- 需要额外绕y轴正方向旋转180度 (0,1,0,0)
		--	base_quaternion[1] = 0
		--	base_quaternion[3] = 1
		-- end

		local device_quaternion = {orient_0, orient_1, orient_2, orient_3}
		if(isFront) then
			device_quaternion[3] = -device_quaternion[3]
			device_quaternion[4] = -device_quaternion[4]
		else
			--
		end
		
		local synthesis_quaternion = quaternionMultiply(base_quaternion,device_quaternion)
		scene:setDeviceOrientation(synthesis_quaternion[1], synthesis_quaternion[2], synthesis_quaternion[3], synthesis_quaternion[4], "1")
		scene:setVisible("2", true)
	end
}
