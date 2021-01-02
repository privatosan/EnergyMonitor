import React from 'react';

import FormControlLabel from '@material-ui/core/FormControlLabel';
import Checkbox from '@material-ui/core/Checkbox';

const source = (props) => {
    return (
        <div>
            <FormControlLabel control={<Checkbox name={props.name} />} label={props.name} />
        </div>
    );
};

export default source;